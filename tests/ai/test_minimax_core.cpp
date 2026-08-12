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
// Tests internes de la fonction minimax() via MasterAITestAccess.
// Note : L'évaluateur renvoyant de grands nombres (ex: 500000 pour un alignement),
// nous utilisons des fenêtres artificiellement basses (ex: [5, 10]) pour forcer 
// les scores à sortir de la fenêtre et tester la logique des bornes (Bounds).
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

// ── Test 1 : Arbre complet sans coupure ─────────────────────────────────────
// Avec une fenêtre infinie [-inf, +inf], aucune coupure n'est possible à la racine.
// On vérifie que la valeur retournée est exacte et stockée avec le flag TTFlag::Exact.
TEST_CASE("[Minimax] core: full-window root returns exact value and stores Exact")
{
	Logger::info("TEST_CASE", "[Minimax] core: full-window root returns exact value and stores Exact");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);


	const int value = Access::search(ai, pos, previousHarmlessMove, 2, NEG_INF, POS_INF);

	// Vérification de la stabilité (déterminisme)
	MasterAI19 ai2 = MasterAI19(2, 1, Color::Black);
	const int value2 = Access::search(ai2, pos, previousHarmlessMove, 2, NEG_INF, POS_INF);
	CHECK(value == value2);

	// Vérification de la Table de Transposition
	const TTEntry* root = Access::tt(ai).probe(pos.zobristHash());
	REQUIRE(root != nullptr);
	CHECK(root->flag == TTFlag::Exact);
	CHECK(root->score == value);
}

// ── Test 2 : Comportement Fail-Soft hors fenêtre ────────────────────────────
// On force une fenêtre très basse [5, 10] sur une position très avantageuse.
// L'algorithme "Fail-Soft" doit renvoyer le vrai score (qui dépasse beta) 
// et non pas être plafonné à 10. La TT doit enregistrer un LowerBound (Fail-High).
TEST_CASE("[Minimax] core: fail-soft returns true value above beta and stores LowerBound")
{
	Logger::info("TEST_CASE", "[Minimax] core: fail-soft returns true value above beta and stores LowerBound");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	// 1. Obtenir la vraie valeur sans restriction
	MasterAI19 aiFull = MasterAI19(1, 1, Color::Black);
	
	const int fullValue = Access::search(aiFull, pos, previousHarmlessMove, 1, NEG_INF, POS_INF);
	CHECK(fullValue > 10); 

	// 2. Chercher avec la fenêtre restreinte [5, 10]
	MasterAI19 aiNarrow = MasterAI19(1, 1, Color::Black);
	const int narrowValue = Access::search(aiNarrow, pos, previousHarmlessMove, 1, 5, 10);

	// 3. Vérifications du Fail-Soft
	CHECK(narrowValue > 10);          // Ne plafonne pas à beta (Fail-Soft)
	CHECK(narrowValue <= fullValue);  // Le fail-high est un minorant du vrai score

	// 4. Vérifications de la Table de Transposition
	const TTEntry* root = Access::tt(aiNarrow).probe(pos.zobristHash());
	REQUIRE(root != nullptr);
	CHECK(root->flag == TTFlag::LowerBound);
	CHECK(root->score == narrowValue);
}