#include "doctest.h"
#include "pattern_fixture.hpp"

// =============================================================================
// check_cross — cross / plus-shape pattern detection
//
// The cross has 5 positions: centre, up (y-1), down (y+1), left (x-1),
// right (x+1).  The score is built as:
//   3 if centre stone present
//   4 per arm stone present
// Returned when score >= 15 (threshold for at least 3 arms + centre or 4 arms).
//
// Opponent scoring (per arm direction, using positions at distance 2 & 3):
//   distance 2 (inner / "INTERN"): +64 = CROSS_OPP_INTERN
//   distance 3 (outer / "EXTERN"): +32 = CROSS_OPP_EXTERN
//   board edge treated like an opponent (position == -1)
// If total opponent score > 64: pattern is skipped (too restricted).
//
// Cross patterns are only built for interior cells (x,y in [1..17]).
// The pattern is indexed for all 5 cells, so any arm position is a valid query.
//
// Return codes (matching the #define constants in PatternTypes.hpp):
//   CROSS_FULL          (19) — centre + 4 arms
//   CROSS_DEMI_NO_MID   (16) — 4 arms, no centre
//   CROSS_DEMI_MID      (15) — centre + 3 arms
//   CROSS_FULL_OPP_EXTERN (51) — full cross + one far opponent
//   CROSS_FULL_OPP_INTERN (83) — full cross + one close opponent
//   0                       — below threshold or skipped
// =============================================================================

// ─── Empty / trivial ─────────────────────────────────────────────────────────

TEST_CASE("cross: empty board returns 0")
{
	t_BWBoard19 board = empty_bb();
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("cross: only 2 arms (score=8 < 15) returns 0")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);  // up
	set_bb19(board.black, 9, 10); // down
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("cross: only centre + 2 arms (score=11 < 15) returns 0")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 9); // centre
	set_bb19(board.black, 9, 8); // up
	set_bb19(board.black, 8, 9); // left
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("cross: cross on board edge (no pattern built) returns 0")
{
	t_BWBoard19 board = empty_bb();
	// Build a cross shape at corner (0,0) — no cross pattern exists there
	set_bb19(board.black, 0, 0);
	set_bb19(board.black, 1, 0);
	set_bb19(board.black, 0, 1);
	CHECK(check_cross_19(board.black, board.white, 0, 0) == 0);
}

// ─── Full cross — CROSS_FULL (19) ────────────────────────────────────────────

TEST_CASE("cross: full cross (centre + 4 arms) returns CROSS_FULL")
{
	//   B          (9,8)
	// B B B        (8,9) (9,9) (10,9)
	//   B          (9,10)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_FULL);
}

TEST_CASE("cross: full cross queried from each arm position")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);

	SUBCASE("query from up arm   (9,8)")
	{
		CHECK(check_cross_19(board.black, board.white, 9, 8) == CROSS_FULL);
	}
	SUBCASE("query from left arm (8,9)")
	{
		CHECK(check_cross_19(board.black, board.white, 8, 9) == CROSS_FULL);
	}
	SUBCASE("query from centre   (9,9)")
	{
		CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_FULL);
	}
	SUBCASE("query from right arm (10,9)")
	{
		CHECK(check_cross_19(board.black, board.white, 10, 9) == CROSS_FULL);
	}
	SUBCASE("query from down arm (9,10)")
	{
		CHECK(check_cross_19(board.black, board.white, 9, 10) == CROSS_FULL);
	}
}

// ─── Partial cross ───────────────────────────────────────────────────────────

TEST_CASE("cross: four arms without centre returns CROSS_DEMI_NO_MID")
{
	// score = 0 + 4*4 = 16
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	// no centre stone
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_DEMI_NO_MID);
}

TEST_CASE("cross: centre + 3 arms (missing down) returns CROSS_DEMI_MID")
{
	// score = 3 + 4*3 = 15
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_DEMI_MID);
}

TEST_CASE("cross: centre + 3 arms (missing left) returns CROSS_DEMI_MID")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	// no left stone
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_DEMI_MID);
}

// ─── Opponent on cross positions ─────────────────────────────────────────────

TEST_CASE("cross: opponent on centre stops detection")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.white, 9, 9); // opponent on centre
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("cross: opponent on up arm stops detection")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 9, 8); // opponent on up arm
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("cross: opponent on right arm stops detection")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.white, 10, 9); // opponent on right arm
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

// ─── Opponent near the cross (modifier scores) ───────────────────────────────

TEST_CASE("cross: full cross + one far opponent (up, distance 3) returns CROSS_FULL_OPP_EXTERN")
{
	// CROSS_FULL_OPP_EXTERN = 19 + 32 = 51
	// For centre at (9,9): opposant_up[0] = (9, y-3) = (9,6)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	set_bb19(board.white, 9, 6); // far up opponent (distance 3)
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_FULL_OPP_EXTERN);
}

TEST_CASE("cross: full cross + one close opponent (up, distance 2) returns CROSS_FULL_OPP_INTERN")
{
	// CROSS_FULL_OPP_INTERN = 19 + 64 = 83
	// For centre at (9,9): opposant_up[1] = (9, y-2) = (9,7)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	set_bb19(board.white, 9, 7); // close up opponent (distance 2)
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_FULL_OPP_INTERN);
}

TEST_CASE("cross: full cross + far opponent on right side returns CROSS_FULL_OPP_EXTERN")
{
	// For centre at (9,9): opposant_right[0] = (x+3, y) = (12,9)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	set_bb19(board.white, 12, 9); // far right opponent (distance 3)
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_FULL_OPP_EXTERN);
}

TEST_CASE("cross: two close opponents (score=128 > 64) causes skip — returns 0")
{
	// One close opponent up AND one close opponent down → 64+64=128 > 64 → skip
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	set_bb19(board.white, 9, 7);  // close up (64)
	set_bb19(board.white, 9, 11); // close down (64)
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("cross: close-up + far-left opponent (score=96 > 64) causes skip — returns 0")
{
	// close up (64) + far left (32) = 96 > 64 → skip
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	set_bb19(board.white, 9, 7); // close up (64)
	set_bb19(board.white, 6, 9); // far left (32)
	CHECK(check_cross_19(board.black, board.white, 9, 9) == 0);
}

// ─── Partial cross with opponent modifier ────────────────────────────────────

TEST_CASE("cross: CROSS_DEMI_NO_MID + far opponent returns CROSS_DEMI_NO_OPP_EXTERN")
{
	// CROSS_DEMI_NO_OPP_EXTERN = 16 + 32 = 48
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	// no centre
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	set_bb19(board.white, 9, 6); // far up opponent
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_DEMI_NO_OPP_EXTERN);
}

TEST_CASE("cross: CROSS_DEMI_MID + far opponent returns CROSS_DEMI_MID_OPP_EXTERN")
{
	// CROSS_DEMI_MID_OPP_EXTERN = 15 + 32 = 47
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	// no down arm
	set_bb19(board.white, 9, 6); // far up opponent
	CHECK(check_cross_19(board.black, board.white, 9, 9) == CROSS_DEMI_MID_OPP_EXTERN);
}

// ─── Color isolation ─────────────────────────────────────────────────────────

TEST_CASE("cross: white full cross detected on white board")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 9, 8);
	set_bb19(board.white, 8, 9);
	set_bb19(board.white, 9, 9);
	set_bb19(board.white, 10, 9);
	set_bb19(board.white, 9, 10);
	CHECK(check_cross_19(board.white, board.black, 9, 9) == CROSS_FULL);
}

TEST_CASE("cross: black stones do not show on white-side query")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 9, 8);
	set_bb19(board.black, 8, 9);
	set_bb19(board.black, 9, 9);
	set_bb19(board.black, 10, 9);
	set_bb19(board.black, 9, 10);
	CHECK(check_cross_19(board.white, board.black, 9, 9) == 0);
}
