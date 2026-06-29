#include "doctest.h"
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/line_helpers.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Tests avancés (edge cases) autour de l'ajustement des scores de mat.
// Le moteur renvoie WIN_SCORE - currentDepth pour une victoire : un mat proche
// (ply faible) vaut donc strictement plus qu'un mat lointain, ce qui force le
// Maximiseur à préférer la victoire la plus rapide.
// ────────────────────────────────────────────────────────────────────────────

static constexpr int kWinScore = 1000000;

static SearchPosition19 posWithSideToMove(GameBoard& b, Color colorToMove)
{
	b.setCurrentColor(colorToMove);
	return SearchPosition19::fromBoard(b);
}

// ── Test 10 : Préférence pour la victoire la plus rapide (Mate Distance) ─────
// Le plateau offre à Noir deux chemins gagnants : un mat immédiat (quatre alignés,
// une seule case complète le cinq) et une ressource plus lente (un trois ouvert
// ailleurs). On vérifie que le Maximiseur choisit le mat en 1 coup, et que le
// score retourné est WIN_SCORE - 1 (ajusté au ply), preuve qu'il n'a pas pris un
// chemin plus profond (qui vaudrait moins).
TEST_CASE("[Minimax] mate distance: maximizer prefers the fastest win")
{
	Logger::info("TEST_CASE", "[Minimax] mate distance: prefer fastest win");

	GameBoard b = empty_board();

	// 1. Chemin gagnant en 1 coup : quatre alignés bloqués à l'arrière => une
	//    unique case (l'extrémité ouverte) complète le cinq.
	const ThreatLine mateInOne{ Direction::E, 3, 5, 4, CellStatus::Black, CellStatus::White, -1 };
	buildThreatLine(b, mateInOne);

	// 2. Ressource gagnante plus lente : un trois ouvert, loin du premier groupe.
	placeRun(b, 3, 12, Direction::E, 3, CellStatus::Black);

	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	// 3. Recherche assez profonde (>= 3) pour « voir » aussi le chemin lent.
	MasterAI19 ai = MasterAI19(3, 1, Color::Black);
	ai.disableTimeLimit();
	const t_cell move = ai.findBestMove(pos, Color::Black);

	// 4. Vérifications : coup gagnant immédiat, et score au ply 1 (mat le plus rapide).
	CHECK(moveIsOneOf(move, winningCells(mateInOne))); // choisit le mat en 1 coup
	CHECK(ai.lastSearchStats().bestScore == kWinScore - 1);
}
