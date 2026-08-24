#include "doctest.h"
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/line_helpers.hpp"
#include "helpers/master_ai_test_access.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Invariance par la Table de Transposition.
// Propriété fondamentale : la TT ne doit jamais changer le résultat d'une
// recherche, seulement sa vitesse. On vérifie donc qu'une table chaude (ou
// pré-remplie par des passes moins profondes) renvoie exactement le même
// score/coup qu'une recherche à froid — en explorant moins de nœuds.
// ────────────────────────────────────────────────────────────────────────────

using Access = MasterAITestAccess<BoardTraits<19> >;

static SearchPosition19 posWithSideToMove(GameBoard& b, Color colorToMove)
{
	b.setCurrentColor(colorToMove);
	return SearchPosition19::fromBoard(b);
}

// ── Invariance : table chaude == même résultat, moins de nœuds ──────────────
// Deux recherches identiques d'affilée (la TT persiste entre les appels) doivent
// donner le même score/coup, la seconde visitant beaucoup moins de nœuds. Après
// clear(), la recherche redevient identique à la passe froide (même comptage).
TEST_CASE("[Minimax] TT invariance: warm table same result, fewer nodes")
{
	Logger::info("TEST_CASE", "[Minimax] TT invariance: warm table fewer nodes");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	MasterAI19 ai = MasterAI19(4, 1, Color::Black);

	// 1. Passe à froid (TT vide).
	const t_cell move1    = ai.findBestMove(pos, Color::Black);
	const int    score1   = ai.lastSearchStats().bestScore;
	const int    visited1 = ai.lastSearchStats().nodesVisited;

	// 2. Passe à chaud (TT remplie par la passe précédente).
	const t_cell move2    = ai.findBestMove(pos, Color::Black);
	const int    score2   = ai.lastSearchStats().bestScore;
	const int    visited2 = ai.lastSearchStats().nodesVisited;

	CHECK(score2 == score1);   // même valeur
	CHECK(move2.x == move1.x); // même coup
	CHECK(move2.y == move1.y);
	CHECK(visited2 < visited1); // « seulement la vitesse »

	// 3. Après clear(), on retombe sur la passe froide à l'identique.
	Access::ttMutable(ai).clear();
	const t_cell move3    = ai.findBestMove(pos, Color::Black);
	const int    score3   = ai.lastSearchStats().bestScore;
	const int    visited3 = ai.lastSearchStats().nodesVisited;

	CHECK(score3 == score1); // résultat inchangé
	CHECK(move3.x == move1.x);
	CHECK(move3.y == move1.y);
	CHECK(visited3 == visited1); // exploration redevenue froide
}

// ── Invariance : pré-remplissage iterative deepening ────────────────────────
// On remplit la TT par des recherches moins profondes (depth 2 puis 3), comme le
// ferait un iterative deepening, avant la recherche profonde (depth 4). Le score
// et le coup doivent être identiques à une recherche froide à depth 4 : les
// entrées peu profondes (réutilisées via le depth-check) ne corrompent rien.
TEST_CASE("[Minimax] TT invariance: iterative-deepening pre-seed keeps the result")
{
	Logger::info("TEST_CASE", "[Minimax] TT invariance: iterative-deepening pre-seed");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	// 1. Référence : recherche froide directe à depth 4.
	MasterAI19   aiRef   = MasterAI19(4, 1, Color::Black);
	const t_cell moveRef = aiRef.findBestMove(pos, Color::Black);
	const int    vRef    = aiRef.lastSearchStats().bestScore;

	// 2. Recherche pré-remplie par paliers 2 → 3 → 4 (même TT, profondeur croissante).
	MasterAI19 ai = MasterAI19(4, 1, Color::Black);

	ai.setSearchDepth(2);
	ai.findBestMove(pos, Color::Black);
	ai.setSearchDepth(3);
	ai.findBestMove(pos, Color::Black);
	ai.setSearchDepth(4);
	const t_cell move = ai.findBestMove(pos, Color::Black);
	const int    v    = ai.lastSearchStats().bestScore;

	// 3. Le pré-remplissage n'a pas changé le résultat profond.
	CHECK(v == vRef);
	CHECK(move.x == moveRef.x);
	CHECK(move.y == moveRef.y);
}
