#include "doctest.h"
#include "pattern_fixture.hpp"

// =============================================================================
// check_four_align — broken-four (four-with-hole) detection
//
// A 5-cell window where exactly 4 cells contain stones and 1 is a hole.
// The hole position determines the return value:
//   1  — hole at window position 1  (B _ B B B)
//   2  — hole at window position 2  (B B _ B B)
//   3  — hole at window position 3  (B B B _ B)
//   0  — no broken-four match
//
// Opponent stone in the hole blocks detection. The pattern is indexed for
// every cell in the 5-cell window, so all 5 query positions are valid.
//
// Note: having all 5 stones still matches (the masks only require 4 of 5),
// so this function fires regardless of whether the hole has a friendly stone.
// =============================================================================

// ─── Empty / trivial ─────────────────────────────────────────────────────────

TEST_CASE("broken4: empty board returns 0")
{
    t_BWBoard19 board = empty_bb();
    CHECK(check_four_align_19(board.black, board.white, 9, 9) == 0);
}

TEST_CASE("broken4: isolated stone alone (no 4-stone window) returns 0")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 9, 9);
    CHECK(check_four_align_19(board.black, board.white, 9, 9) == 0);
}

// ─── Hole at position 1 — B _ B B B ──────────────────────────────────────────

TEST_CASE("broken4: B _ B B B (hole pos 1) returns 1 — all 5 query positions")
{
    // Window (3,5)-(7,5): stones at 3, 5, 6, 7; hole at 4
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    // hole at (4, 5)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);

    SUBCASE("query from stone at x=3") { CHECK(check_four_align_19(board.black, board.white, 3, 5) == 1); }
    SUBCASE("query from hole  at x=4") { CHECK(check_four_align_19(board.black, board.white, 4, 5) == 1); }
    SUBCASE("query from stone at x=5") { CHECK(check_four_align_19(board.black, board.white, 5, 5) == 1); }
    SUBCASE("query from stone at x=6") { CHECK(check_four_align_19(board.black, board.white, 6, 5) == 1); }
    SUBCASE("query from stone at x=7") { CHECK(check_four_align_19(board.black, board.white, 7, 5) == 1); }
}

TEST_CASE("broken4: opponent at hole pos 1 blocks detection")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.white, 4, 5);  // opponent fills hole
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    CHECK(check_four_align_19(board.black, board.white, 5, 5) == 0);
}

// ─── Hole at position 2 — B B _ B B ──────────────────────────────────────────

TEST_CASE("broken4: B B _ B B (hole pos 2) returns 2 — all 5 query positions")
{
    // Window (3,5)-(7,5): stones at 3, 4, 6, 7; hole at 5
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.black, 4, 5);
    // hole at (5, 5)
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);

    SUBCASE("query from stone at x=3") { CHECK(check_four_align_19(board.black, board.white, 3, 5) == 2); }
    SUBCASE("query from stone at x=4") { CHECK(check_four_align_19(board.black, board.white, 4, 5) == 2); }
    SUBCASE("query from hole  at x=5") { CHECK(check_four_align_19(board.black, board.white, 5, 5) == 2); }
    SUBCASE("query from stone at x=6") { CHECK(check_four_align_19(board.black, board.white, 6, 5) == 2); }
    SUBCASE("query from stone at x=7") { CHECK(check_four_align_19(board.black, board.white, 7, 5) == 2); }
}

TEST_CASE("broken4: opponent at hole pos 2 blocks detection")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.black, 4, 5);
    set_bb19(board.white, 5, 5);  // opponent fills hole
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    CHECK(check_four_align_19(board.black, board.white, 6, 5) == 0);
}

// ─── Hole at position 3 — B B B _ B ──────────────────────────────────────────

TEST_CASE("broken4: B B B _ B (hole pos 3) returns 3 — all 5 query positions")
{
    // Window (3,5)-(7,5): stones at 3, 4, 5, 7; hole at 6
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.black, 4, 5);
    set_bb19(board.black, 5, 5);
    // hole at (6, 5)
    set_bb19(board.black, 7, 5);

    SUBCASE("query from stone at x=3") { CHECK(check_four_align_19(board.black, board.white, 3, 5) == 3); }
    SUBCASE("query from stone at x=4") { CHECK(check_four_align_19(board.black, board.white, 4, 5) == 3); }
    SUBCASE("query from stone at x=5") { CHECK(check_four_align_19(board.black, board.white, 5, 5) == 3); }
    SUBCASE("query from hole  at x=6") { CHECK(check_four_align_19(board.black, board.white, 6, 5) == 3); }
    SUBCASE("query from stone at x=7") { CHECK(check_four_align_19(board.black, board.white, 7, 5) == 3); }
}

TEST_CASE("broken4: opponent at hole pos 3 blocks detection")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.black, 4, 5);
    set_bb19(board.black, 5, 5);
    set_bb19(board.white, 6, 5);  // opponent fills hole
    set_bb19(board.black, 7, 5);
    CHECK(check_four_align_19(board.black, board.white, 5, 5) == 0);
}

// ─── Vertical ────────────────────────────────────────────────────────────────

TEST_CASE("broken4: vertical B _ B B B returns 1")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 5, 3);
    // hole at (5, 4)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 5, 6);
    set_bb19(board.black, 5, 7);
    CHECK(check_four_align_19(board.black, board.white, 5, 5) == 1);
}

TEST_CASE("broken4: vertical B B _ B B returns 2")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 5, 3);
    set_bb19(board.black, 5, 4);
    // hole at (5, 5)
    set_bb19(board.black, 5, 6);
    set_bb19(board.black, 5, 7);
    CHECK(check_four_align_19(board.black, board.white, 5, 5) == 2);
}

// ─── Diagonal backslash (\) ───────────────────────────────────────────────────

TEST_CASE("broken4: diagonal-backslash B _ B B B returns 1")
{
    // (3,3) _ (5,5) (6,6) (7,7)
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 3);
    // hole at (4, 4)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 6);
    set_bb19(board.black, 7, 7);
    CHECK(check_four_align_19(board.black, board.white, 5, 5) == 1);
}

TEST_CASE("broken4: diagonal-backslash B B _ B B returns 2")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 3);
    set_bb19(board.black, 4, 4);
    // hole at (5, 5)
    set_bb19(board.black, 6, 6);
    set_bb19(board.black, 7, 7);
    CHECK(check_four_align_19(board.black, board.white, 6, 6) == 2);
}

// ─── Diagonal slash (/) ──────────────────────────────────────────────────────

TEST_CASE("broken4: diagonal-slash B _ B B B returns 1")
{
    // (7,3) _ (5,5) (4,6) (3,7)
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 7, 3);
    // hole at (6, 4)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 4, 6);
    set_bb19(board.black, 3, 7);
    CHECK(check_four_align_19(board.black, board.white, 5, 5) == 1);
}

// ─── Color isolation ─────────────────────────────────────────────────────────

TEST_CASE("broken4: white B B _ B B detected on white board")
{
    t_BWBoard19 board = empty_bb();
    set_bb19(board.white, 3, 5);
    set_bb19(board.white, 4, 5);
    // hole at (5, 5)
    set_bb19(board.white, 6, 5);
    set_bb19(board.white, 7, 5);
    CHECK(check_four_align_19(board.white, board.black, 5, 5) == 2);
}
