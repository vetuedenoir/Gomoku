#include "doctest.h"
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "optimization/TranspositionTable.hpp"
#include "helpers/master_ai_test_access.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Indépendance au chemin (transpositions).
// Une même position atteinte par deux ordres de coups différents doit produire
// le même hash Zobrist (XOR commutatif + même nombre de demi-coups), donc viser
// le même slot de TT. C'est la prémisse de la réutilisation par transposition.
// ────────────────────────────────────────────────────────────────────────────

using Access = MasterAITestAccess<BoardTraits<19>>;

// ── Transposition : ordres différents, même hash et même entrée TT ──────────
TEST_CASE("[TT] transposition: different move orders share hash and TT entry")
{
	// 1. Même position finale, deux ordres de coups différents (couleurs B,W,B).
	GameBoard b1(19, Color::Black);
	SearchPosition19 p1 = SearchPosition19::fromBoard(b1);
	p1.makeMove(5, 5, CellStatus::Black);
	p1.makeMove(6, 6, CellStatus::White);
	p1.makeMove(7, 7, CellStatus::Black);

	GameBoard b2(19, Color::Black);
	SearchPosition19 p2 = SearchPosition19::fromBoard(b2);
	p2.makeMove(7, 7, CellStatus::Black);
	p2.makeMove(6, 6, CellStatus::White);
	p2.makeMove(5, 5, CellStatus::Black);

	// 2. Les hash coïncident : même position, indépendamment de l'ordre.
	CHECK(p1.zobristHash() == p2.zobristHash());

	// 3. Bout-en-bout : une valeur cachée pour p1 est retrouvée via p2.
	MasterAI19 ai = MasterAI19(2, 1, Color::Black);

	Access::ttMutable(ai).store(p1.zobristHash(), 777, 5, TTFlag::Exact,
	                            { -1, -1 });

	const TTEntry* hit = Access::tt(ai).probe(p2.zobristHash());
	REQUIRE(hit != nullptr);
	CHECK(hit->score == 777);
}
