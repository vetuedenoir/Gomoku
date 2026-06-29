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
// Tests de consultation de la Table de Transposition (TT Probe).
// On pilote minimax() via MasterAITestAccess et on s'appuie sur lastSearchStats()
// pour distinguer un hit instantané (un seul nœud visité) d'une recherche complète.
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

// Réutilisation d'une valeur Exacte ──────────────────────────────
// Première passe : on évalue le sous-arbre complet (fenêtre infinie => racine
// stockée en Exact). Seconde passe sur le même état et à profondeur égale : le
// probe renvoie la valeur immédiatement, sans ré-explorer le sous-arbre.
TEST_CASE("[Minimax] TT: exact hit returns cached value without re-exploring")
{
	Logger::info("TEST_CASE", "[Minimax] TT: exact hit returns cached value");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);


	// 1. Première recherche : remplit la TT et explore tout le sous-arbre.
	const int cached = Access::search(ai, pos, previousHarmlessMove, 2, NEG_INF, POS_INF, true);
	const int visitedAfterFirst = ai.lastSearchStats().nodesVisited;
	const int hitsAfterFirst     = ai.lastSearchStats().ttHits;
	CHECK(visitedAfterFirst > 1); // un vrai sous-arbre a bien été parcouru

	// 2. Seconde recherche, même hash et même profondeur : hit Exact instantané.
	const int reused = Access::search(ai, pos, previousHarmlessMove, 2, NEG_INF, POS_INF, true);
	const int visitedAfterSecond = ai.lastSearchStats().nodesVisited;
	const int hitsAfterSecond     = ai.lastSearchStats().ttHits;

	// 3. Vérifications : même valeur, et seul le nœud racine est revisité.
	CHECK(reused == cached);
	CHECK(visitedAfterSecond - visitedAfterFirst == 1); // racine seule, aucun enfant
	CHECK(hitsAfterSecond - hitsAfterFirst == 1);       // le probe a bien servi
}

// Rejet pour profondeur insuffisante (Depth Check) ───────────────
// On injecte une entrée Exact peu profonde (depth = 2) avec un score sentinelle,
// puis on lance la recherche à depth = 4. La condition entry.depth >= depth
// échoue (2 < 4) : la TT est ignorée et la recherche complète est effectuée.
TEST_CASE("[Minimax] TT: shallow entry is rejected, full search runs")
{
	Logger::info("TEST_CASE", "[Minimax] TT: shallow entry rejected (depth check)");

	GameBoard b = empty_board();
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	SearchPosition19 pos = posWithSideToMove(b, Color::Black);

	// 1. Référence : recherche complète à depth = 4 sans entrée injectée.
	MasterAI19 aiRef = MasterAI19(4, 1, Color::Black);
	const int refValue   = Access::search(aiRef, pos, previousHarmlessMove, 4, NEG_INF, POS_INF, true);
	const int refVisited = aiRef.lastSearchStats().nodesVisited;
	const int refHits    = aiRef.lastSearchStats().ttHits; // hits internes légitimes (transpositions)

	// 2. Injection d'une entrée Exact peu profonde (depth = 2) au hash de la racine.
	MasterAI19 ai = MasterAI19(4, 1, Color::Black);

	const int sentinel = 123456; // valeur que la vraie recherche ne produit jamais
	Access::ttMutable(ai).store(pos.zobristHash(), sentinel, 2, TTFlag::Exact,
	                            { -1, -1, CellStatus::Empty });

	// 3. Recherche à depth = 4 : l'entrée depth = 2 doit être rejetée.
	const int value = Access::search(ai, pos, previousHarmlessMove, 4, NEG_INF, POS_INF, true);

	// 4. Vérifications : la sentinelle est ignorée et la recherche complète a tourné.
	CHECK(value != sentinel);                                   // valeur cachée non utilisée
	CHECK(value == refValue);                                   // résultat identique à la référence
	CHECK(ai.lastSearchStats().nodesVisited == refVisited);     // exploration complète
	CHECK(ai.lastSearchStats().ttHits == refHits);              // l'entrée injectée n'ajoute aucun hit

	// La recherche a remplacé l'entrée peu profonde par son propre résultat (depth = 4).
	const TTEntry* root = Access::tt(ai).probe(pos.zobristHash());
	REQUIRE(root != nullptr);
	CHECK(root->depth == 4);
}
