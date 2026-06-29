#include "doctest.h"
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "optimization/TranspositionTable.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/line_helpers.hpp"
#include "helpers/master_ai_test_access.hpp"
#include <limits>

// ────────────────────────────────────────────────────────────────────────────
// Tests d'ajustement de fenêtre au probe TT (LowerBound monte alpha, UpperBound
// descend beta). On injecte des entrées à la main puis on observe soit une coupure
// instantanée (un seul nœud visité), soit un simple rétrécissement de fenêtre.
// ────────────────────────────────────────────────────────────────────────────

using Access = MasterAITestAccess<BoardTraits<19>>;

static constexpr int NEG_INF = std::numeric_limits<int>::min();
static constexpr int POS_INF = std::numeric_limits<int>::max();

static SearchPosition19 posWithSideToMove(GameBoard& b, Color colorToMove)
{
	b.setCurrentColor(colorToMove);
	return SearchPosition19::fromBoard(b);
}

// Coup neutre utilisé uniquement pour la vérification de victoire au nœud.
static const t_cell previousHarmlessMove = { 0, 0 };

//  Coupure instantanée via LowerBound ─────────────────────────────
// LowerBound = 15 injecté. Fenêtre [5, 10] : le probe monte alpha à max(5, 15) = 15,
// constate 15 >= 10 et coupe immédiatement, sans explorer le moindre enfant.
TEST_CASE("[Minimax] TT: LowerBound raises alpha and cuts instantly")
{
	Logger::info("TEST_CASE", "[Minimax] TT: LowerBound instant cutoff");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);
	ai.disableTimeLimit();
	Access::ttMutable(ai).store(pos.zobristHash(), 15, 2, TTFlag::LowerBound,
	                            { -1, -1, CellStatus::Empty });

	const int value = Access::search(ai, pos, previousHarmlessMove, 2, 5, 10, true);

	const SearchStats& stats = ai.lastSearchStats();
	CHECK(value == 15);                 // borne renvoyée telle quelle
	CHECK(stats.ttHits == 1);           // le probe a servi
	CHECK(stats.nodesVisited == 1);     // racine seule : aucune exploration
	CHECK(stats.nodesEvaluated == 0);   // aucune feuille évaluée
	CHECK(stats.nodesPruned == 0);      // coupure TT, pas une coupure de boucle
}

// Coupure instantanée via UpperBound ─────────────────────────────
// UpperBound = 2 injecté. Fenêtre [5, 10] : le probe descend beta à min(10, 2) = 2,
// constate alpha (5) >= beta (2) et coupe immédiatement.
TEST_CASE("[Minimax] TT: UpperBound lowers beta and cuts instantly")
{
	Logger::info("TEST_CASE", "[Minimax] TT: UpperBound instant cutoff");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);
	ai.disableTimeLimit();
	Access::ttMutable(ai).store(pos.zobristHash(), 2, 2, TTFlag::UpperBound,
	                            { -1, -1, CellStatus::Empty });

	const int value = Access::search(ai, pos, previousHarmlessMove, 2, 5, 10, true);

	const SearchStats& stats = ai.lastSearchStats();
	CHECK(value == 2);                  // borne renvoyée telle quelle
	CHECK(stats.ttHits == 1);           // le probe a servi
	CHECK(stats.nodesVisited == 1);     // racine seule : aucune exploration
	CHECK(stats.nodesEvaluated == 0);   // aucune feuille évaluée
	CHECK(stats.nodesPruned == 0);      // coupure TT, pas une coupure de boucle
}

// Rétrécissement de la fenêtre sans coupure instantanée ──────────
// UpperBound injecté pour resserrer beta sans déclencher de coupure au probe :
// la recherche continue, mais un coup fort déclenche désormais une coupure Beta
// impossible avec le beta d'origine.
// (Note d'échelle : les feuilles ne valent jamais ~10 ; on transpose donc le
// scénario [0, 20] / UpperBound 8 / coup 10 sur l'échelle réelle du moteur, en
// dérivant les bornes de la vraie valeur du nœud `best`.)
TEST_CASE("[Minimax] TT: UpperBound shrinks window and enables a later beta-cutoff")
{
	Logger::info("TEST_CASE", "[Minimax] TT: UpperBound shrinks window");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	// 1. Découvrir la vraie valeur du nœud (meilleur enfant) en fenêtre complète.
	MasterAI19 aiProbe = MasterAI19(1, 1, Color::Black);
	aiProbe.disableTimeLimit();
	const int best = Access::search(aiProbe, pos, previousHarmlessMove, 1, NEG_INF, POS_INF, true);
	REQUIRE(best > 1); // marge pour placer alpha=0 < (best-1)

	const int wideBeta     = best + 1;  // « beta original » : best ne coupe pas
	const int injectedUpper = best - 1; // beta resserré : best coupe désormais

	// 2. Référence sans injection, fenêtre [0, best+1] : aucune coupure.
	MasterAI19 aiRef = MasterAI19(1, 1, Color::Black);
	aiRef.disableTimeLimit();
	const int refValue = Access::search(aiRef, pos, previousHarmlessMove, 1, 0, wideBeta, true);
	CHECK(aiRef.lastSearchStats().nodesPruned == 0); // best < beta original => pas de coupure

	// 3. Avec UpperBound = best-1 : le probe descend beta, la recherche continue,
	//    puis le coup valant `best` provoque la coupure Beta.
	MasterAI19 ai = MasterAI19(1, 1, Color::Black);
	ai.disableTimeLimit();
	Access::ttMutable(ai).store(pos.zobristHash(), injectedUpper, 1, TTFlag::UpperBound,
	                            { -1, -1, CellStatus::Empty });
	const int value = Access::search(ai, pos, previousHarmlessMove, 1, 0, wideBeta, true);

	const SearchStats& stats = ai.lastSearchStats();
	CHECK(stats.ttHits == 1);          // le probe a servi
	CHECK(stats.nodesVisited > 1);     // pas de coupure instantanée : la recherche a continué
	CHECK(stats.nodesPruned == 1);     // coupure Beta rendue possible par le rétrécissement
	CHECK(value == refValue);          // même valeur fail-soft que la référence
}
