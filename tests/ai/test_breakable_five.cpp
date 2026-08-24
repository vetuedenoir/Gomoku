#include "doctest.h"

#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/master_ai_test_access.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Règle de la capture finale, côté moteur.
//
// Un cinq n'est terminal que si l'adversaire ne peut pas le casser à la prise.
// Le moteur doit donc : ne plus annoncer mat sur un cinq réfutable, continuer à
// annoncer mat sur un cinq imprenable, et — en défense — trouver la prise qui
// casse une ligne déjà posée plutôt que de la subir.
// ────────────────────────────────────────────────────────────────────────────

static constexpr int kWinScore = 1000000;

namespace
{

// Noir a quatre pierres sur la ligne 5 : (7,5) complète le cinq.
void black_four_on_row5(GameBoard& b)
{
	for (int x = 3; x <= 6; ++x)
		place(b, x, 5, CellStatus::Black);
}

// La paire (5,5)+(5,6) est prenable par Blanc en (5,4) : toute ligne passant
// par (5,5) — donc tout cinq de la ligne 5 — est cassable.
void white_can_break_row5(GameBoard& b)
{
	place(b, 5, 6, CellStatus::Black);
	place(b, 5, 7, CellStatus::White);
}

SearchPosition19 positionOf(GameBoard& b, Color toMove,
                            int capturesByBlack = 0, int capturesByWhite = 0)
{
	b.setCurrentColor(toMove);
	return SearchPosition19::fromBoard(b, capturesByBlack, capturesByWhite);
}

} // namespace

// ─── Offense ─────────────────────────────────────────────────────────────────

TEST_CASE("[Minimax] an unbreakable five is still announced as mate")
{
	GameBoard b = empty_board();
	black_four_on_row5(b);

	SearchPosition19 pos = positionOf(b, Color::Black);
	MasterAI19 ai(1, 1, Color::Black);

	const t_cell move = ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().bestScore == kWinScore - 1);
	// (2,5) et (7,5) complètent tous deux la ligne.
	CHECK((move.x == 7 || move.x == 2));
	CHECK(move.y == 5);
}

TEST_CASE("[Minimax] a breakable five is not announced as mate")
{
	// Même quatre, mais Blanc dispose de la prise qui casserait la ligne :
	// le raccourci racine ne doit plus se déclencher, et la recherche ne doit
	// pas rapporter un score de mat.
	GameBoard b = empty_board();
	black_four_on_row5(b);
	white_can_break_row5(b);

	SearchPosition19 pos = positionOf(b, Color::Black);
	MasterAI19 ai(1, 1, Color::Black);

	ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().bestScore < kWinScore - 10);
}

TEST_CASE("[Minimax] the same four is mate again once the capture is gone")
{
	// Contrôle du test précédent : on retire la seule pierre blanche qui
	// rendait la prise possible, tout le reste est identique.
	GameBoard b = empty_board();
	black_four_on_row5(b);
	place(b, 5, 6, CellStatus::Black);   // la paire reste, mais sans ancre blanche

	SearchPosition19 pos = positionOf(b, Color::Black);
	MasterAI19 ai(1, 1, Color::Black);

	ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().bestScore == kWinScore - 1);
}

// ─── Défense ─────────────────────────────────────────────────────────────────

TEST_CASE("[Minimax] the engine spends its move breaking an opposing five")
{
	// Noir a cinq alignés sur la ligne 5 ; la partie continue parce que Blanc
	// peut prendre (5,5)+(5,6) en jouant (5,4). Noir peut ensuite rejouer (5,5)
	// et gagner quand même — la prise ne fait que repousser l'échéance — mais
	// c'est le seul coup qui ne perd pas immédiatement, et la distance au mat
	// doit suffire à le faire préférer à tout le reste.
	GameBoard b = empty_board();
	for (int x = 3; x <= 7; ++x)
		place(b, x, 5, CellStatus::Black);
	white_can_break_row5(b);

	SearchPosition19 pos = positionOf(b, Color::White);
	MasterAI19 ai(2, 1, Color::White);

	const t_cell move = ai.findBestMove(pos, Color::White);

	CHECK(move.x == 5);
	CHECK(move.y == 4);
}

TEST_CASE("[Minimax] ignoring an opposing five is scored as a loss")
{
	// Même position, mais on force un coup quelconque : le sursis se résout au
	// ply suivant et le score doit être un mat négatif pour l'IA (Blanc).
	GameBoard b = empty_board();
	for (int x = 3; x <= 7; ++x)
		place(b, x, 5, CellStatus::Black);
	white_can_break_row5(b);

	SearchPosition19 pos = positionOf(b, Color::White);
	MasterAI19 ai(2, 1, Color::White);

	const std::optional<PendingWin> pending =
		findExistingFive<BoardTraits<19>>(pos.board(), Color::Black);
	REQUIRE(pending.has_value());

	// Blanc joue loin de la ligne ; le nœud enfant hérite du sursis.
	const EvaluatedMove quiet =
		MasterAITestAccess<BoardTraits<19>>::lightKey(ai, pos.board(), {15, 15}, Color::White, 0);
	const MoveStateHash hash = pos.buildMoveHash(quiet, Color::White);
	pos.makeMove(15, 15, Color::White, hash);

	// Profondeur 0 : on isole la résolution du sursis, sans laisser Noir rejouer.
	const int score = MasterAITestAccess<BoardTraits<19>>::searchWithPending(
		ai, pos, {15, 15}, 0,
		std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), pending);

	CHECK(score <= -(kWinScore - 10));
}

TEST_CASE("[Minimax] breaking the line clears the pending mate")
{
	// Identique, sauf que Blanc joue la prise : le même nœud ne doit plus
	// résoudre le sursis en mat.
	GameBoard b = empty_board();
	for (int x = 3; x <= 7; ++x)
		place(b, x, 5, CellStatus::Black);
	white_can_break_row5(b);

	SearchPosition19 pos = positionOf(b, Color::White);
	MasterAI19 ai(2, 1, Color::White);

	const std::optional<PendingWin> pending =
		findExistingFive<BoardTraits<19>>(pos.board(), Color::Black);
	REQUIRE(pending.has_value());

	const EvaluatedMove capture =
		MasterAITestAccess<BoardTraits<19>>::lightKey(ai, pos.board(), {5, 4}, Color::White, 0);
	REQUIRE(capture.captureMask != 0);

	const MoveStateHash hash = pos.buildMoveHash(capture, Color::White);
	pos.makeMove(5, 4, Color::White, hash);

	const int score = MasterAITestAccess<BoardTraits<19>>::searchWithPending(
		ai, pos, {5, 4}, 0,
		std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), pending);

	CHECK(score > -(kWinScore - 10));
}

// ─── Une prise qui ne casse pas la ligne ne compte pas ───────────────────────

TEST_CASE("[Minimax] the tenth capture does not save against a five it does not touch")
{
	// Noir a un cinq imprenable sur la ligne 5. Blanc, à huit pierres prises,
	// a une prise ailleurs sur le plateau : elle le porterait à dix, mais elle
	// ne touche pas l'alignement, donc elle ne le sauve pas. Le raccourci
	// racine de la dixième capture doit être neutralisé et la recherche doit
	// rapporter une position perdue pour Blanc.
	GameBoard b = empty_board();
	for (int x = 3; x <= 7; ++x)
		place(b, x, 5, CellStatus::Black);

	place(b, 12, 12, CellStatus::Black);
	place(b, 13, 13, CellStatus::Black);
	place(b, 14, 14, CellStatus::White);

	SearchPosition19 pos = positionOf(b, Color::White, /*byBlack=*/0, /*byWhite=*/8);
	MasterAI19 ai(2, 1, Color::White);

	ai.findBestMove(pos, Color::White);

	CHECK(ai.lastSearchStats().bestScore <= -(kWinScore - 10));
}

// ─── Nulle : la prise casse la ligne ET porte le défenseur à dix ─────────────

TEST_CASE("[Minimax] a break that reaches ten captures scores a draw")
{
	// Noir complète son cinq en (7,5). Blanc est à huit pierres prises et
	// dispose de la prise (5,4) qui casse la ligne : elle le porte à dix. Les
	// deux victoires tombent sur le même coup → la position vaut nulle, ni mat
	// ni cinq cassable ordinaire.
	GameBoard b = empty_board();
	black_four_on_row5(b);
	white_can_break_row5(b);

	SearchPosition19 pos = positionOf(b, Color::Black, /*byBlack=*/0, /*byWhite=*/8);
	MasterAI19 ai(2, 1, Color::Black);

	ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().bestScore == 0);
}

TEST_CASE("[Minimax] the same break below ten is only a pending five")
{
	// Position identique, mais Blanc n'est qu'à six pierres prises : la prise
	// casse la ligne sans le porter à dix. Ce n'est donc pas nulle — le cinq
	// reste en sursis et le score doit rester strictement positif pour Noir.
	GameBoard b = empty_board();
	black_four_on_row5(b);
	white_can_break_row5(b);

	SearchPosition19 pos = positionOf(b, Color::Black, /*byBlack=*/0, /*byWhite=*/6);
	MasterAI19 ai(2, 1, Color::Black);

	ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().bestScore > 0);
	CHECK(ai.lastSearchStats().bestScore < kWinScore - 10);
}
