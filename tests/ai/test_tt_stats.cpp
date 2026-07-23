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
// Vérifie que les compteurs de stats reflètent le travail de la TT aux DEUX
// niveaux :
//   - minimax (nœuds internes) : ttStores / ttHits
//   - racine (findBestMove)    : ttRootHits / ttRootOrderingHits / ttRootExactSeeds
//
// On distingue les niveaux car ce sont des chemins de code séparés :
//   * minimax() écrit et relit la TT (probe/store internes).
//   * findBestMove() ne consulte la TT QUE pour l'ordonnancement / le seeding,
//     sans jamais couper tôt.
// ────────────────────────────────────────────────────────────────────────────

using Access = MasterAITestAccess<BoardTraits<19>>;

static constexpr int NEG_INF = std::numeric_limits<int>::min();
static constexpr int POS_INF = std::numeric_limits<int>::max();

static const t_cell previousHarmlessMove = { 0, 0 };

static SearchPosition19 threatPos(GameBoard& b)
{
	buildThreatLine(b, { Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 });
	b.setCurrentColor(Color::Black);
	return SearchPosition19::fromBoard(b);
}

// ── Niveau minimax : ttStores puis ttHits ───────────────────────────────────
// La 1re recherche remplit la TT (ttStores > 0). La 2nde, sur le même état et à
// la même profondeur, retrouve au moins l'entrée racine du sous-arbre (ttHits++).
// NB : minimax() ne remet PAS _stats à zéro (seul findBestMove le fait), donc on
// raisonne par différence entre les deux appels.
TEST_CASE("[TT-stats][minimax] stores then hits are counted")
{
	GameBoard b = empty_board();
	SearchPosition19 pos = threatPos(b);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);

	Access::search(ai, pos, previousHarmlessMove, 2, NEG_INF, POS_INF);
	const int stores1 = ai.lastSearchStats().ttStores;
	const int hits1   = ai.lastSearchStats().ttHits;

	Access::search(ai, pos, previousHarmlessMove, 2, NEG_INF, POS_INF);
	const int hits2   = ai.lastSearchStats().ttHits;

	CHECK(stores1 > 0);          // le sous-arbre a écrit des entrées
	CHECK(hits2 - hits1 >= 1);   // le 2nd passage a réutilisé au moins la racine
}

// ── Niveau racine : aucune entrée → aucun hit racine ────────────────────────
// findBestMove() ne stocke jamais le hash de la racine elle-même ; sur une TT
// vierge, le probe racine ne peut donc pas toucher.
TEST_CASE("[TT-stats][root] empty table yields no root hit")
{
	GameBoard b = empty_board();
	SearchPosition19 pos = threatPos(b);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);
	const t_cell best = ai.findBestMove(pos, Color::Black);

	const SearchStats& s = ai.lastSearchStats();
	CHECK(s.ttRootHits == 0);
	CHECK(s.ttRootOrderingHits == 0);
	CHECK(s.ttRootExactSeeds == 0);
	REQUIRE(best.x >= 0); // un coup légal a bien été renvoyé
}

// ── Niveau racine : une entrée EXACT injectée est comptée ───────────────────
// On injecte au hash de la racine une entrée EXACT (depth >= maxDepth) dont le
// bestMove est un vrai candidat racine. findBestMove doit alors :
//   - compter le hit racine                (ttRootHits)
//   - trouver ce bestMove et le mettre en tête (ttRootOrderingHits)
//   - amorcer bestScore/bestMove            (ttRootExactSeeds)
// … sans jamais couper tôt (la boucle explore quand même les candidats).
TEST_CASE("[TT-stats][root] exact injected entry is counted (hit + ordering + seed)")
{
	GameBoard b = empty_board();
	SearchPosition19 pos = threatPos(b);

	// Coup racine légal de référence, obtenu sur une TT vierge.
	MasterAI19 aiRef = MasterAI19(2, 1, Color::Black);
	const t_cell best = aiRef.findBestMove(pos, Color::Black);
	REQUIRE(best.x >= 0);

	// Entrée EXACT injectée au hash racine, bestMove = un candidat réel.
	MasterAI19 ai = MasterAI19(2, 1, Color::Black);
	Access::ttMutable(ai).store(pos.zobristHash(), 123, /*depth*/ 2, TTFlag::Exact, best);

	ai.findBestMove(pos, Color::Black);

	const SearchStats& s = ai.lastSearchStats();
	CHECK(s.ttRootHits == 1);
	CHECK(s.ttRootOrderingHits == 1);
	CHECK(s.ttRootExactSeeds == 1);
}

// ── Niveau racine : une entrée trop peu profonde ne « seed » pas ────────────
// Même injection, mais depth < maxDepth : le bestMove sert quand même à
// l'ordonnancement (hit + ordering), mais le score EXACT n'amorce PAS bestScore.
TEST_CASE("[TT-stats][root] shallow exact entry orders but does not seed")
{
	GameBoard b = empty_board();
	SearchPosition19 pos = threatPos(b);

	MasterAI19 aiRef = MasterAI19(4, 1, Color::Black);
	const t_cell best = aiRef.findBestMove(pos, Color::Black);
	REQUIRE(best.x >= 0);

	MasterAI19 ai = MasterAI19(4, 1, Color::Black);
	Access::ttMutable(ai).store(pos.zobristHash(), 123, /*depth*/ 2, TTFlag::Exact, best);

	ai.findBestMove(pos, Color::Black);

	const SearchStats& s = ai.lastSearchStats();
	CHECK(s.ttRootHits == 1);
	CHECK(s.ttRootOrderingHits == 1);
	CHECK(s.ttRootExactSeeds == 0); // depth(2) < maxDepth(4) → pas de seeding
}

// ── Les compteurs sont remis à zéro à chaque findBestMove ───────────────────
// _tt persiste entre les appels, mais les stats doivent refléter UN seul search.
TEST_CASE("[TT-stats] counters reset each findBestMove call")
{
	GameBoard b = empty_board();
	SearchPosition19 pos = threatPos(b);

	MasterAI19 ai = MasterAI19(2, 1, Color::Black);
	ai.findBestMove(pos, Color::Black);
	const int stores1 = ai.lastSearchStats().ttStores;

	ai.findBestMove(pos, Color::Black);
	const int stores2 = ai.lastSearchStats().ttStores;

	CHECK(stores1 > 0);
	// Sans le reset, stores2 accumulerait stores1 ; avec reset il reste du même
	// ordre de grandeur (et ne double pas).
	CHECK(stores2 <= stores1);
}
