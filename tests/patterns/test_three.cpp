#include "doctest.h"
#include "pattern_fixture.hpp"

// =============================================================================
// check_three_align — open-three detection
//
// A 4-cell window: stone_pos[0..3].  Three stones form a "three" in two ways:
//   Full three:    pos[0], pos[1], pos[2] all set (no hole needed)
//   Three hole-1:  pos[0], pos[2], pos[3] set; pos[1] (hole) free of opponent
//   Three hole-2:  pos[0], pos[1], pos[3] set; pos[2] (hole) free of opponent
//
// Opponent modifier (left side, from inner to outer):
//   oposant_pos[1] (close-left, distance 1): adds 256 = SCORE_OPP_INTERN
//   oposant_pos[0] (far-left,   distance 2): adds 128 = SCORE_OPP_EXTERN
// Same for right side with oposant_pos[2] and [3].
//
// If combined opponent score > 256 (both sides present) the pattern is skipped.
//
// Return values for a single three (not a double):
//   SCORE_3_FULL    (8)   — full three, no opponent
//   SCORE_3_HOLE   (12)   — hole three, no opponent
//   SCORE_FULL_EXTERN (136) — full three + one far opponent
//   SCORE_HOLE_EXTERN (140) — hole three + one far opponent
//   SCORE_FULL_INTERN (264) — full three + one close opponent
//   SCORE_HOLE_INTERN (268) — hole three + one close opponent
//   0                     — blocked (opponent on both sides) or no match
//
// Double-three return values (total / 4, when two threes intersect):
//   SCORE_DOUBLE_FULL_FULL (4), SCORE_DOUBLE_HOLE_FULL (5), SCORE_DOUBLE_HOLE_HOLE (6)
// =============================================================================

// ─── Empty / trivial ─────────────────────────────────────────────────────────

TEST_CASE("three: empty board returns 0")
{
	t_BWBoard19 board = empty_bb();
	CHECK(check_three_align_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("three: only 2 stones does not trigger")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == 0);
}

// ─── Full three — no opponent ─────────────────────────────────────────────────

TEST_CASE("three: horizontal full three in the clear returns SCORE_3_FULL")
{
	// [ ] B B B [ ]  — completely open
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_3_FULL);
}

TEST_CASE("three: vertical full three in the clear returns SCORE_3_FULL")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 5, 4);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 5, 6);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_3_FULL);
}

TEST_CASE("three: diagonal-backslash full three in the clear returns SCORE_3_FULL")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 4);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 6);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_3_FULL);
}

TEST_CASE("three: diagonal-slash full three in the clear returns SCORE_3_FULL")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 6, 4);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 4, 6);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_3_FULL);
}

// ─── Hole three — no opponent ─────────────────────────────────────────────────

TEST_CASE("three: hole-three B _ B B (hole pos 1) returns SCORE_3_HOLE")
{
	// Pattern x=4: stones at (4,5),(6,5),(7,5), hole at (5,5)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	// hole at (5, 5)
	set_bb19(board.black, 6, 5);
	set_bb19(board.black, 7, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_3_HOLE);
}

TEST_CASE("three: hole-three B B _ B (hole pos 2) returns SCORE_3_HOLE")
{
	// Pattern x=4: stones at (4,5),(5,5),(7,5), hole at (6,5)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	// hole at (6, 5)
	set_bb19(board.black, 7, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_3_HOLE);
}

TEST_CASE("three: opponent at hole pos 1 blocks hole-three detection")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.white, 5, 5); // opponent fills hole
	set_bb19(board.black, 6, 5);
	set_bb19(board.black, 7, 5);
	CHECK(check_three_align_19(board.black, board.white, 6, 5) == 0);
}

// ─── Opponent modifier — one side ─────────────────────────────────────────────

TEST_CASE("three: full three + far-left opponent returns SCORE_FULL_EXTERN")
{
	// W · B B B ·  — white 2 cells to the left
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 2, 5); // far left (oposant_pos[0] for window starting at x=4)
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_FULL_EXTERN);
}

TEST_CASE("three: full three + far-right opponent returns SCORE_FULL_EXTERN")
{
	// · B B B · W  — white 2 cells to the right
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	set_bb19(board.white, 8, 5); // far right (oposant_pos[3] for window at x=4)
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_FULL_EXTERN);
}

TEST_CASE("three: full three + close-left opponent returns SCORE_FULL_INTERN")
{
	// · W B B B ·  — white 1 cell to the left
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 3, 5); // close left (oposant_pos[1])
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_FULL_INTERN);
}

TEST_CASE("three: full three + close-right opponent returns SCORE_FULL_INTERN")
{
	// · B B B W ·  — white 1 cell to the right
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	set_bb19(board.white, 7, 5); // close right (oposant_pos[2])
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_FULL_INTERN);
}

TEST_CASE("three: hole-three + far-left opponent returns SCORE_HOLE_EXTERN")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 2, 5);
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	// hole at (6, 5)
	set_bb19(board.black, 7, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_HOLE_EXTERN);
}

TEST_CASE("three: hole-three + close-left opponent returns SCORE_HOLE_INTERN")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 3, 5);
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	// hole at (6, 5)
	set_bb19(board.black, 7, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_HOLE_INTERN);
}

// ─── Blocking — two-sided opponent ───────────────────────────────────────────

TEST_CASE("three: close opponent on both sides returns 0 (blocked)")
{
	// W B B B W  — score = 256+256 = 512 > 256 → skipped
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 3, 5);
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	set_bb19(board.white, 7, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == 0);
}

TEST_CASE("three: close-left + far-right opponent returns 0 (blocked)")
{
	// score = 256 (close-left) + 128 (far-right) = 384 > 256 → skipped
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 3, 5);
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	set_bb19(board.white, 8, 5);
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == 0);
}

// ─── Double-three ─────────────────────────────────────────────────────────────
//
// When two three-patterns intersect at the queried cell (in different directions),
// the function counts them both, then returns (total_score / 4).

TEST_CASE("three: double full-full (H+V) returns SCORE_DOUBLE_FULL_FULL")
{
	// Horizontal: (4,5),(5,5),(6,5)
	// Vertical:   (5,3),(5,4),(5,5)
	// Intersection at (5,5)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	set_bb19(board.black, 5, 3);
	set_bb19(board.black, 5, 4);
	// total_score = 8+8 = 16, returns 16/4 = 4
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_DOUBLE_FULL_FULL);
}

TEST_CASE("three: double hole+full (H hole + V full) returns SCORE_DOUBLE_HOLE_FULL")
{
	// Horizontal hole-three: (4,5),(5,5),(7,5) — hole at (6,5)
	// Vertical full three:   (5,3),(5,4),(5,5)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	// hole at (6, 5)
	set_bb19(board.black, 7, 5);
	set_bb19(board.black, 5, 3);
	set_bb19(board.black, 5, 4);
	// total_score = 12+8 = 20, returns 20/4 = 5
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_DOUBLE_HOLE_FULL);
}

TEST_CASE("three: double hole+hole (H hole + V hole) returns SCORE_DOUBLE_HOLE_HOLE")
{
	// Horizontal hole-three: (4,5),(5,5),(7,5) — hole at (6,5)
	// Vertical hole-three:   (5,3),(5,5),(5,6) — hole at (5,4)
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	// hole at (6, 5)
	set_bb19(board.black, 7, 5);
	set_bb19(board.black, 5, 3);
	// hole at (5, 4)
	set_bb19(board.black, 5, 6);
	// total_score = 12+12 = 24, returns 24/4 = 6
	CHECK(check_three_align_19(board.black, board.white, 5, 5) == SCORE_DOUBLE_HOLE_HOLE);
}

// ─── Color isolation ─────────────────────────────────────────────────────────

TEST_CASE("three: white full three returns SCORE_3_FULL on white board")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.white, 4, 5);
	set_bb19(board.white, 5, 5);
	set_bb19(board.white, 6, 5);
	CHECK(check_three_align_19(board.white, board.black, 5, 5) == SCORE_3_FULL);
}

TEST_CASE("three: black stones do not show on white-side query")
{
	t_BWBoard19 board = empty_bb();
	set_bb19(board.black, 4, 5);
	set_bb19(board.black, 5, 5);
	set_bb19(board.black, 6, 5);
	CHECK(check_three_align_19(board.white, board.black, 5, 5) == 0);
}
