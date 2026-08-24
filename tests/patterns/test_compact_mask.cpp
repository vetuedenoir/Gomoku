#include "doctest.h"
#include "pattern_fixture.hpp"
#include "bitboard/CompactMask.hpp"
#include "bitboard/PatternTypes.hpp"

// Lot 2 infrastructure: CompactMask + direction windows.
// Existing pattern tables / check_* are unchanged; this single test validates
// that windows are well-formed and that Lot-2 popcount thresholds would not
// false-negative on a representative board for every pattern family.

namespace {

int maxPopOverDirs(const typename TestTraits::Bitboard& stones, int x, int y)
{
	auto& tool = patternTool();
	int best = 0;
	for (int dir = 0; dir < 4; ++dir)
	{
		const int n = popWindow(tool.directionWindow(x, y, dir), stones);
		if (n > best)
			best = n;
	}
	return best;
}

int popOnDir(const typename TestTraits::Bitboard& stones, int x, int y, int dir)
{
	return popWindow(patternTool().directionWindow(x, y, dir), stones);
}

} // namespace

TEST_CASE("CompactMask: windows cover all pattern families without false-negative thresholds")
{
	auto& tool = patternTool();

	// ── 1. Every cell × direction fits in CompactMask (≤ 2 limbs) ───────────
	for (int y = 0; y < TestTraits::BOARD_SIZE; ++y)
	{
		for (int x = 0; x < TestTraits::BOARD_SIZE; ++x)
		{
			for (int dir = 0; dir < 4; ++dir)
			{
				const CompactMask<TestTraits>& w = tool.directionWindow(x, y, dir);
				CHECK(w.words >= 1);
				CHECK(w.words <= CompactMask<TestTraits>::MAX_WORDS);
				CHECK(w.w + w.words <= TestTraits::WORD_COUNT);
			}
		}
	}

	// Centre horizontal window: 9 cells (k=-4..4). May span 1–2 limbs if the
	// row crosses a 64-bit boundary (STRIDE packing).
	{
		const auto& w = tool.directionWindow(9, 9, DIR_HORIZ);
		int bits = 0;
		for (uint8_t i = 0; i < w.words; ++i)
			bits += __builtin_popcountll(w.m[i]);
		CHECK(bits == 9);
	}

	// Diagonal at centre needs more than 2 limbs on 19×19 (STRIDE_D step).
	{
		const auto& w = tool.directionWindow(9, 9, DIR_DIAG_D);
		CHECK(w.words >= 2);
		CHECK(w.words <= CompactMask<TestTraits>::MAX_WORDS);
	}

	// ── 2. CompactMask::from + popWindow basics ─────────────────────────────
	{
		typename TestTraits::Bitboard bb {};
		set_bb19(bb, 5, 5);
		set_bb19(bb, 6, 5);
		set_bb19(bb, 7, 5);
		const CompactMask<TestTraits> cm = CompactMask<TestTraits>::from(bb);
		CHECK(cm.words == 1);
		CHECK(popWindow(cm, bb) == 3);

		typename TestTraits::Bitboard empty {};
		CHECK(popWindow(cm, empty) == 0);
	}

	// ── 3. Per-family: check_* hits ⇒ window popcount ≥ Lot-2 threshold ─────
	// Thresholds assume the query stone is already on `stones` (same as rawShapeScore).

	// five / is_five_in_a_row — need ≥ 5 on some direction
	{
		t_BWBoard19 board = empty_bb();
		for (int x = 3; x <= 7; ++x)
			set_bb19(board.black, x, 5);
		REQUIRE(isWin_ultra_19(board.black, 5, 5));
		CHECK(popOnDir(board.black, 5, 5, DIR_HORIZ) >= 5);
	}

	// open four — need ≥ 4
	{
		t_BWBoard19 board = empty_bb();
		for (int x = 3; x <= 6; ++x)
			set_bb19(board.black, x, 5);
		REQUIRE(is_Open_4_19(board.black, board.white, 5, 5) == 2);
		CHECK(popOnDir(board.black, 5, 5, DIR_HORIZ) >= 4);
	}

	// broken four — need ≥ 4
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 3, 5);
		set_bb19(board.black, 5, 5);
		set_bb19(board.black, 6, 5);
		set_bb19(board.black, 7, 5);
		REQUIRE(check_four_align_19(board.black, board.white, 5, 5) == 1);
		CHECK(popOnDir(board.black, 5, 5, DIR_HORIZ) >= 4);
	}

	// open three — need ≥ 3
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 5, 5);
		set_bb19(board.black, 6, 5);
		set_bb19(board.black, 7, 5);
		REQUIRE(check_three_align_19(board.black, board.white, 5, 5) != 0);
		CHECK(popOnDir(board.black, 5, 5, DIR_HORIZ) >= 3);
	}

	// super four — need ≥ 4 (actually 5 stones in the 7-window; ±4 covers them)
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 3, 5);
		set_bb19(board.black, 5, 5);
		set_bb19(board.black, 6, 5);
		set_bb19(board.black, 7, 5);
		set_bb19(board.black, 9, 5);
		REQUIRE(check_super4_19(board.black, board.white, 5, 5) == 1);
		CHECK(popOnDir(board.black, 5, 5, DIR_HORIZ) >= 4);
	}

	// cross — multi-dir; Lot-2 plan allows a global neighbourhood filter.
	// At least one axis must show ≥ 3 own stones around the centre for CROSS_DEMI_MID+.
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 9, 9);  // centre
		set_bb19(board.black, 9, 8);  // N
		set_bb19(board.black, 9, 10); // S
		set_bb19(board.black, 8, 9);  // W
		REQUIRE(check_cross_19(board.black, board.white, 9, 9) != 0);
		CHECK(maxPopOverDirs(board.black, 9, 9) >= 3);
	}

	// Sparse cell: thresholds correctly *fail* (safe to skip direction loops)
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 9, 9);
		CHECK(popOnDir(board.black, 9, 9, DIR_HORIZ) < 3);
		CHECK(popOnDir(board.black, 9, 9, DIR_VERT) < 3);
		CHECK(isWin_ultra_19(board.black, 9, 9) == 0);
		CHECK(is_Open_4_19(board.black, board.white, 9, 9) == 0);
		CHECK(check_four_align_19(board.black, board.white, 9, 9) == 0);
		CHECK(check_three_align_19(board.black, board.white, 9, 9) == 0);
	}
}

// Hot path (prefilter) must match *_reference oracles on every family.
TEST_CASE("CompactMask: filtered check_* ≡ *_reference")
{
	auto& tool = patternTool();

	auto expectEqual = [&](typename TestTraits::Bitboard& own,
	                       typename TestTraits::Bitboard& opp, int x, int y) {
		CHECK(tool.is_five_in_a_row(own, x, y)
		      == tool.is_five_in_a_row_reference(own, x, y));
		CHECK(tool.check_open_four(own, opp, x, y)
		      == tool.check_open_four_reference(own, opp, x, y));
		CHECK(tool.check_broken_four(own, opp, x, y)
		      == tool.check_broken_four_reference(own, opp, x, y));
		CHECK(tool.check_open_three(own, opp, x, y)
		      == tool.check_open_three_reference(own, opp, x, y));
		CHECK(tool.check_open_three_filtered(own, opp, x, y)
		      == tool.check_open_three_reference(own, opp, x, y));
		CHECK(tool.check_super_four(own, opp, x, y)
		      == tool.check_super_four_reference(own, opp, x, y));
		CHECK(tool.check_cross(own, opp, x, y)
		      == tool.check_cross_reference(own, opp, x, y));
	};

	// Hand boards (stone already on `own`, as in rawShapeScore).
	{
		t_BWBoard19 board = empty_bb();
		expectEqual(board.black, board.white, 9, 9);
	}
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 5, 5);
		set_bb19(board.black, 6, 5);
		set_bb19(board.black, 7, 5);
		expectEqual(board.black, board.white, 5, 5);
		expectEqual(board.black, board.white, 6, 5);
		expectEqual(board.black, board.white, 7, 5);
	}
	{
		t_BWBoard19 board = empty_bb();
		for (int x = 3; x <= 7; ++x)
			set_bb19(board.black, x, 5);
		expectEqual(board.black, board.white, 5, 5);
	}
	{
		t_BWBoard19 board = empty_bb();
		for (int x = 3; x <= 6; ++x)
			set_bb19(board.black, x, 5);
		expectEqual(board.black, board.white, 5, 5);
	}
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 3, 5);
		set_bb19(board.black, 5, 5);
		set_bb19(board.black, 6, 5);
		set_bb19(board.black, 7, 5);
		expectEqual(board.black, board.white, 5, 5);
	}
	{
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 3, 5);
		set_bb19(board.black, 5, 5);
		set_bb19(board.black, 6, 5);
		set_bb19(board.black, 7, 5);
		set_bb19(board.black, 9, 5);
		expectEqual(board.black, board.white, 5, 5);
	}
	{
		// Vertical three + far opponent
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 8, 4);
		set_bb19(board.black, 8, 5);
		set_bb19(board.black, 8, 6);
		set_bb19(board.white, 8, 2);
		expectEqual(board.black, board.white, 8, 5);
	}
	{
		// Diagonal SE three
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 6, 6);
		set_bb19(board.black, 7, 7);
		set_bb19(board.black, 8, 8);
		expectEqual(board.black, board.white, 7, 7);
	}
	{
		// Cross (+): centre + N/S/W
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 9, 9);
		set_bb19(board.black, 9, 8);
		set_bb19(board.black, 9, 10);
		set_bb19(board.black, 8, 9);
		expectEqual(board.black, board.white, 9, 9);
	}
	{
		// Cross DEMI_NO_MID: four arms, empty centre — global filter must keep.
		t_BWBoard19 board = empty_bb();
		set_bb19(board.black, 9, 8);
		set_bb19(board.black, 9, 10);
		set_bb19(board.black, 8, 9);
		set_bb19(board.black, 10, 9);
		expectEqual(board.black, board.white, 9, 8);
		expectEqual(board.black, board.white, 9, 9);
	}
	{
		// Dense-ish: three + noise on other axes
		t_BWBoard19 board = empty_bb();
		for (int x = 4; x <= 6; ++x)
			set_bb19(board.black, x, 10);
		set_bb19(board.black, 10, 3);
		set_bb19(board.white, 10, 4);
		expectEqual(board.black, board.white, 5, 10);
	}

	// Light random sample: place the query stone, sprinkle neighbours.
	for (unsigned seed = 1; seed <= 200; ++seed)
	{
		t_BWBoard19 board = empty_bb();
		const int qx = static_cast<int>(seed % 19);
		const int qy = static_cast<int>((seed * 7) % 19);
		set_bb19(board.black, qx, qy);
		unsigned s = seed * 2654435761u;
		for (int k = 0; k < 12; ++k)
		{
			s = s * 1664525u + 1013904223u;
			const int x = static_cast<int>(s % 19);
			const int y = static_cast<int>((s >> 8) % 19);
			if ((s & 1u) != 0)
				set_bb19(board.black, x, y);
			else
				set_bb19(board.white, x, y);
		}
		set_bb19(board.black, qx, qy);
		clear_bit_generic<TestTraits>(board.white, qx, qy);
		expectEqual(board.black, board.white, qx, qy);
	}
}
