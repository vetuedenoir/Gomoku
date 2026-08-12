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
// Tests d'élagage Alpha-Beta via MasterAITestAccess.
// Astuce : on place la borne hors de portée de toutes les feuilles (scores dans
// [-1000000, +1000000]) pour garantir une coupure dès le premier coup, puis on
// lit lastSearchStats() pour prouver que les coups suivants ne sont pas évalués.
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

// ── Test 3 : Coupure Beta (Fail-High) côté Maximiseur ───────────────────────
// beta est placé sous le plancher des scores : le premier coup évalué dépasse
// donc beta et déclenche la coupure. On vérifie qu'un seul coup est évalué (les
// suivants sont ignorés) et que le nœud est enregistré en LowerBound.
TEST_CASE("[Minimax] pruning: MAX beta-cutoff evaluates one move and stores LowerBound")
{
	Logger::info("TEST_CASE", "[Minimax] pruning: MAX beta-cutoff stores LowerBound");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	// 1. Référence en fenêtre complète : sans coupure, tous les coups sont évalués.
	MasterAI19 aiFull = MasterAI19(1, 1, Color::Black);
	
	Access::search(aiFull, pos, previousHarmlessMove, 1, NEG_INF, POS_INF);
	CHECK(aiFull.lastSearchStats().nodesEvaluated > 1);

	// 2. Recherche MAX avec beta sous toutes les valeurs possibles
	MasterAI19 ai = MasterAI19(1, 1, Color::Black);

	const int value = Access::search(ai, pos, previousHarmlessMove, 1, NEG_INF, -2000000);

	// 3. Vérification de l'élagage
	const SearchStats& stats = ai.lastSearchStats();
	CHECK(stats.nodesPruned == 1);     // une seule coupure
	CHECK(stats.nodesEvaluated == 1);  // les coups suivants ne sont pas évalués

	// 4. Vérification de la Table de Transposition
	const TTEntry* root = Access::tt(ai).probe(pos.zobristHash());
	REQUIRE(root != nullptr);
	CHECK(root->flag == TTFlag::LowerBound);  // Fail-High
	CHECK(root->score == value);
}

// ── Test 4 : Coupure Alpha (Fail-Low) côté Minimiseur ───────────────────────
// Symétrique : alpha est placé au-dessus de toutes les valeurs possibles. Le
// premier coup évalué par MIN passe sous alpha et stoppe la boucle. Le nœud est
// enregistré en UpperBound.
TEST_CASE("[Minimax] pruning: MIN alpha-cutoff evaluates one move and stores UpperBound")
{
	Logger::info("TEST_CASE", "[Minimax] pruning: MIN alpha-cutoff stores UpperBound");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::White); // MIN = adversaire au trait

	// 1. Référence en fenêtre complète : sans coupure, tous les coups sont évalués.
	MasterAI19 aiFull = MasterAI19(1, 1, Color::Black);
	
	Access::search(aiFull, pos, previousHarmlessMove, 1, NEG_INF, POS_INF);
	CHECK(aiFull.lastSearchStats().nodesEvaluated > 1);

	// 2. Recherche MIN avec alpha au-dessus de toutes les valeurs possibles
	MasterAI19 ai = MasterAI19(1, 1, Color::Black);

	const int value = Access::search(ai, pos, previousHarmlessMove, 1, 2000000, POS_INF);

	// 3. Vérification de l'élagage
	const SearchStats& stats = ai.lastSearchStats();
	CHECK(stats.nodesPruned == 1);     // une seule coupure
	CHECK(stats.nodesEvaluated == 1);  // arrêt immédiat de la boucle

	// 4. Vérification de la Table de Transposition
	const TTEntry* root = Access::tt(ai).probe(pos.zobristHash());
	REQUIRE(root != nullptr);
	CHECK(root->flag == TTFlag::UpperBound);  // Fail-Low
	CHECK(root->score == value);
}
