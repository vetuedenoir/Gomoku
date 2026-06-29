#include "doctest.h"
#include "optimization/TranspositionTable.hpp"
#include "game/contracts/contracts.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Sécurité anti-collision de la TT (table en accès direct : index = hash & mask).
// Deux clés qui ne diffèrent que par un bit hors du masque d'index tombent dans
// le même slot. Le garde sur zobristKey doit empêcher tout faux positif : un
// probe d'une clé qui collisionne sans correspondre doit renvoyer nullptr.
// ────────────────────────────────────────────────────────────────────────────

// ── Collision : clé non concordante dans le même slot → nullptr ─────────────
TEST_CASE("[TT] collision: mismatching key in the same slot returns nullptr")
{
	TranspositionTable tt;

	const uint64_t keyA = 0x0123456789ABCDEFull;
	const uint64_t keyB = keyA ^ (1ull << 63); // même slot (bit 63 hors du masque), clé différente

	tt.store(keyA, 42, 3, TTFlag::Exact, { -1, -1, CellStatus::Empty });

	// 1. La clé stockée se relit correctement.
	const TTEntry* a = tt.probe(keyA);
	REQUIRE(a != nullptr);
	CHECK(a->score == 42);

	// 2. La clé qui collisionne mais ne correspond pas est rejetée (pas de faux hit).
	CHECK(tt.probe(keyB) == nullptr);

	// 3. Écrire keyB écrase le slot ; keyA n'est alors plus retrouvée.
	tt.store(keyB, 99, 3, TTFlag::Exact, { -1, -1, CellStatus::Empty });
	CHECK(tt.probe(keyA) == nullptr);
	const TTEntry* bptr = tt.probe(keyB);
	REQUIRE(bptr != nullptr);
	CHECK(bptr->score == 99);
}
