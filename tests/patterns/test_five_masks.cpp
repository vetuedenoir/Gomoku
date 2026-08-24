#include "doctest.h"
#include "pattern_fixture.hpp"

#include <vector>

// =============================================================================
// find_five_masks — same matching as is_five_in_a_row, but returns the cells
// of every matched alignment. Used by the endgame-capture rule, which must know
// which stones form the five before asking whether a pair inside it is takeable.
// =============================================================================

using BB19 = typename TestTraits::Bitboard;

namespace
{

	struct Cell
	{
		int x, y;
	};

	int find_masks(const BB19& stones, int x, int y, BB19* out, int maxOut = BitboardTool19::MAX_FIVE_MASKS)
	{
		return patternTool().find_five_masks(stones, x, y, out, maxOut);
	}

	bool mask_is_exactly(const BB19& mask, const std::vector<Cell>& cells)
	{
		if (popcount_bb(mask) != static_cast<int>(cells.size()))
			return false;
		for (const Cell& c : cells)
		{
			if (!get_bb_generic<TestTraits>(mask, c.x, c.y))
				return false;
		}
		return true;
	}

	bool any_mask_is_exactly(const BB19* masks, int count, const std::vector<Cell>& cells)
	{
		for (int i = 0; i < count; ++i)
		{
			if (mask_is_exactly(masks[i], cells))
				return true;
		}
		return false;
	}

} // namespace

// ─── No five ─────────────────────────────────────────────────────────────────

TEST_CASE("five_masks: empty board returns no mask")
{
	t_BWBoard19 board = empty_bb();
	BB19        masks[BitboardTool19::MAX_FIVE_MASKS];

	CHECK(find_masks(board.black, 9, 9, masks) == 0);
}

TEST_CASE("five_masks: 4 in a row returns no mask")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 6; x++)
		set_bb19(board.black, x, 5);

	BB19 masks[BitboardTool19::MAX_FIVE_MASKS];
	CHECK(find_masks(board.black, 5, 5, masks) == 0);
}

// ─── Exact five: one mask, the five cells ────────────────────────────────────

TEST_CASE("five_masks: horizontal five yields exactly its own cells")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 7; x++)
		set_bb19(board.black, x, 5);

	const std::vector<Cell> line = { { 3, 5 }, { 4, 5 }, { 5, 5 }, { 6, 5 }, { 7, 5 } };
	BB19                    masks[BitboardTool19::MAX_FIVE_MASKS];

	SUBCASE("queried from the first stone")
	{
		const int n = find_masks(board.black, 3, 5, masks);
		REQUIRE(n == 1);
		CHECK(mask_is_exactly(masks[0], line));
	}
	SUBCASE("queried from the middle stone")
	{
		const int n = find_masks(board.black, 5, 5, masks);
		REQUIRE(n == 1);
		CHECK(mask_is_exactly(masks[0], line));
	}
	SUBCASE("queried from the last stone")
	{
		const int n = find_masks(board.black, 7, 5, masks);
		REQUIRE(n == 1);
		CHECK(mask_is_exactly(masks[0], line));
	}
}

TEST_CASE("five_masks: vertical five yields exactly its own cells")
{
	t_BWBoard19 board = empty_bb();
	for (int y = 3; y <= 7; y++)
		set_bb19(board.black, 5, y);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = find_masks(board.black, 5, 5, masks);

	REQUIRE(n == 1);
	CHECK(mask_is_exactly(masks[0], { { 5, 3 }, { 5, 4 }, { 5, 5 }, { 5, 6 }, { 5, 7 } }));
}

TEST_CASE("five_masks: diagonal-backslash five yields exactly its own cells")
{
	t_BWBoard19 board = empty_bb();
	for (int i = 0; i < 5; i++)
		set_bb19(board.black, 3 + i, 3 + i);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = find_masks(board.black, 5, 5, masks);

	REQUIRE(n == 1);
	CHECK(mask_is_exactly(masks[0], { { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 } }));
}

TEST_CASE("five_masks: diagonal-slash five yields exactly its own cells")
{
	t_BWBoard19 board = empty_bb();
	for (int i = 0; i < 5; i++)
		set_bb19(board.black, 7 - i, 3 + i);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = find_masks(board.black, 5, 5, masks);

	REQUIRE(n == 1);
	CHECK(mask_is_exactly(masks[0], { { 7, 3 }, { 6, 4 }, { 5, 5 }, { 4, 6 }, { 3, 7 } }));
}

TEST_CASE("five_masks: five on the board edge is found")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 0; x <= 4; x++)
		set_bb19(board.black, x, 5);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = find_masks(board.black, 2, 5, masks);

	REQUIRE(n == 1);
	CHECK(mask_is_exactly(masks[0], { { 0, 5 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 } }));
}

// ─── Several fives through the same stone ────────────────────────────────────
//
// The endgame-capture rule wins as soon as ONE alignment is unbreakable, so
// every candidate must be reported, not just the first.

TEST_CASE("five_masks: six in a row yields both sub-alignments")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 8; x++)
		set_bb19(board.black, x, 5);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = find_masks(board.black, 5, 5, masks);

	REQUIRE(n == 2);
	CHECK(any_mask_is_exactly(masks, n, { { 3, 5 }, { 4, 5 }, { 5, 5 }, { 6, 5 }, { 7, 5 } }));
	CHECK(any_mask_is_exactly(masks, n, { { 4, 5 }, { 5, 5 }, { 6, 5 }, { 7, 5 }, { 8, 5 } }));
}

TEST_CASE("five_masks: crossing fives yield one mask per direction")
{
	// A horizontal and a vertical five sharing the stone (5,5).
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 7; x++)
		set_bb19(board.black, x, 5);
	for (int y = 3; y <= 7; y++)
		set_bb19(board.black, 5, y);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = find_masks(board.black, 5, 5, masks);

	REQUIRE(n == 2);
	CHECK(any_mask_is_exactly(masks, n, { { 3, 5 }, { 4, 5 }, { 5, 5 }, { 6, 5 }, { 7, 5 } }));
	CHECK(any_mask_is_exactly(masks, n, { { 5, 3 }, { 5, 4 }, { 5, 5 }, { 5, 6 }, { 5, 7 } }));
}

TEST_CASE("five_masks: maxOut caps the number of masks written")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 8; x++)
		set_bb19(board.black, x, 5);

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS] = {};
	const int n                                     = find_masks(board.black, 5, 5, masks, /*maxOut=*/1);

	REQUIRE(n == 1);
	CHECK(popcount_bb(masks[0]) == 5);
	// The second slot must be left untouched.
	CHECK(popcount_bb(masks[1]) == 0);
}

// ─── Colour isolation ────────────────────────────────────────────────────────

TEST_CASE("five_masks: a black five yields nothing on the white board")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 7; x++)
		set_bb19(board.black, x, 5);

	BB19 masks[BitboardTool19::MAX_FIVE_MASKS];
	CHECK(find_masks(board.white, 5, 5, masks) == 0);
}

TEST_CASE("five_masks: a white stone interrupting the run kills the mask")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 6; x++)
		set_bb19(board.black, x, 5);
	set_bb19(board.white, 7, 5);

	BB19 masks[BitboardTool19::MAX_FIVE_MASKS];
	CHECK(find_masks(board.black, 5, 5, masks) == 0);
}

// ─── Agreement with the existing predicates ──────────────────────────────────

TEST_CASE("five_masks: agrees with is_five_in_a_row over a swept board")
{
	// A five sliding along row 9, queried from every cell of the row: the two
	// predicates must never disagree, and every mask must be a real alignment
	// fully occupied by the queried colour.
	for (int start = 0; start + 5 <= 19; ++start)
	{
		t_BWBoard19 board = empty_bb();
		for (int i = 0; i < 5; i++)
			set_bb19(board.black, start + i, 9);

		for (int x = 0; x < 19; ++x)
		{
			BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
			const int n = find_masks(board.black, x, 9, masks);

			CHECK((n > 0) == patternTool().is_five_in_a_row(board.black, x, 9));

			for (int i = 0; i < n; ++i)
			{
				CHECK(popcount_bb(masks[i]) == 5);
				CHECK(get_bb_generic<TestTraits>(masks[i], x, 9));
				for (int w = 0; w < TestTraits::WORD_COUNT; ++w)
					CHECK((masks[i][w] & ~board.black[w]) == 0ULL);
			}
		}
	}
}

TEST_CASE("five_masks: prefiltered and reference variants agree")
{
	t_BWBoard19 board = empty_bb();
	for (int x = 3; x <= 8; x++)
		set_bb19(board.black, x, 5);
	for (int y = 3; y <= 7; y++)
		set_bb19(board.black, 5, y);

	for (int x = 0; x < 19; ++x)
	{
		for (int y = 0; y < 19; ++y)
		{
			BB19 fast[BitboardTool19::MAX_FIVE_MASKS];
			BB19 ref[BitboardTool19::MAX_FIVE_MASKS];

			const int nFast = patternTool().find_five_masks(board.black, x, y, fast, BitboardTool19::MAX_FIVE_MASKS);
			const int nRef =
				patternTool().find_five_masks_reference(board.black, x, y, ref, BitboardTool19::MAX_FIVE_MASKS);

			REQUIRE(nFast == nRef);
			for (int i = 0; i < nFast; ++i)
				CHECK(fast[i] == ref[i]);
		}
	}
}
