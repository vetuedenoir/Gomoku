#include "doctest.h"
#include "pattern_fixture.hpp"

// =============================================================================
// check_super4 — double-open broken-four detection
//
// Pattern: B _ B B B _ B  (7-cell window, holes at positions 1 and 5)
//
// The mask has 5 stones: positions 0, 2, 3, 4, 6 in the window.
// Detection requires:
//   1. All 5 stone positions occupied by boardA.
//   2. Neither hole is occupied by boardB (opponent).
//
// Returns 1 on detection, 0 otherwise.
// The pattern is indexed for all 7 cells in the window.
// =============================================================================

// ─── Empty / trivial ─────────────────────────────────────────────────────────

TEST_CASE("super4: empty board returns 0")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    CHECK(check_super4_19(t.lts4, board.black, board.white, 9, 9) == 0);
}

TEST_CASE("super4: only 4 stones (missing one anchor) returns 0")
{
    // B _ B B B _ missing the rightmost anchor at (9,5)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    // hole at (4, 5)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    // hole at (8, 5) — no stone at (9, 5)
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 0);
}

// ─── Horizontal ──────────────────────────────────────────────────────────────

TEST_CASE("super4: horizontal B _ B B B _ B — detected from every cell in window")
{
    // Window (3,5)-(9,5): stones at 3,5,6,7,9; holes at 4 and 8
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    // hole at (4, 5)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    // hole at (8, 5)
    set_bb19(board.black, 9, 5);

    SUBCASE("query from stone at x=3") { CHECK(check_super4_19(t.lts4, board.black, board.white, 3, 5) == 1); }
    SUBCASE("query from hole  at x=4") { CHECK(check_super4_19(t.lts4, board.black, board.white, 4, 5) == 1); }
    SUBCASE("query from stone at x=5") { CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 1); }
    SUBCASE("query from stone at x=6") { CHECK(check_super4_19(t.lts4, board.black, board.white, 6, 5) == 1); }
    SUBCASE("query from stone at x=7") { CHECK(check_super4_19(t.lts4, board.black, board.white, 7, 5) == 1); }
    SUBCASE("query from hole  at x=8") { CHECK(check_super4_19(t.lts4, board.black, board.white, 8, 5) == 1); }
    SUBCASE("query from stone at x=9") { CHECK(check_super4_19(t.lts4, board.black, board.white, 9, 5) == 1); }
}

TEST_CASE("super4: opponent at hole 1 blocks detection")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.white, 4, 5);  // opponent in hole 1
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    // hole at (8, 5) is free
    set_bb19(board.black, 9, 5);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 0);
}

TEST_CASE("super4: opponent at hole 2 blocks detection")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    // hole 1 at (4, 5) is free
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    set_bb19(board.white, 8, 5);  // opponent in hole 2
    set_bb19(board.black, 9, 5);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 0);
}

TEST_CASE("super4: opponent in both holes blocks detection")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.white, 4, 5);
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    set_bb19(board.white, 8, 5);
    set_bb19(board.black, 9, 5);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 6, 5) == 0);
}

// ─── Vertical ────────────────────────────────────────────────────────────────

TEST_CASE("super4: vertical B _ B B B _ B returns 1")
{
    // Window (5,3)-(5,9): stones at y=3,5,6,7,9; holes at y=4 and y=8
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 5, 3);
    // hole at (5, 4)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 5, 6);
    set_bb19(board.black, 5, 7);
    // hole at (5, 8)
    set_bb19(board.black, 5, 9);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 1);
}

TEST_CASE("super4: vertical — opponent at hole blocks detection")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 5, 3);
    set_bb19(board.white, 5, 4);  // blocks hole 1
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 5, 6);
    set_bb19(board.black, 5, 7);
    // hole at (5, 8)
    set_bb19(board.black, 5, 9);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 0);
}

// ─── Diagonal backslash (\) ───────────────────────────────────────────────────

TEST_CASE("super4: diagonal-backslash B _ B B B _ B returns 1")
{
    // Window (3,3)-(9,9): stones at (3,3),(5,5),(6,6),(7,7),(9,9)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 3);
    // hole at (4, 4)
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 6);
    set_bb19(board.black, 7, 7);
    // hole at (8, 8)
    set_bb19(board.black, 9, 9);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 1);
}

TEST_CASE("super4: diagonal-backslash — opponent at hole blocks detection")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 3);
    set_bb19(board.white, 4, 4);  // blocks hole 1
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 6);
    set_bb19(board.black, 7, 7);
    // hole at (8, 8)
    set_bb19(board.black, 9, 9);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 5, 5) == 0);
}

// ─── Diagonal slash (/) ──────────────────────────────────────────────────────

TEST_CASE("super4: diagonal-slash B _ B B B _ B returns 1")
{
    // Slash: stride 19. Start at (9,3): (9,3),(8,4),(7,5),(6,6),(5,7),(4,8),(3,9)
    // Stones at positions 0,2,3,4,6; holes at 1 and 5
    // Pos 0=(9,3), hole 1=(8,4), pos 2=(7,5), pos 3=(6,6), pos 4=(5,7), hole 5=(4,8), pos 6=(3,9)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 9, 3);
    // hole at (8, 4)
    set_bb19(board.black, 7, 5);
    set_bb19(board.black, 6, 6);
    set_bb19(board.black, 5, 7);
    // hole at (4, 8)
    set_bb19(board.black, 3, 9);
    CHECK(check_super4_19(t.lts4, board.black, board.white, 7, 5) == 1);
}

// ─── Color isolation ─────────────────────────────────────────────────────────

TEST_CASE("super4: white B _ B B B _ B detected on white board")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.white, 3, 5);
    set_bb19(board.white, 5, 5);
    set_bb19(board.white, 6, 5);
    set_bb19(board.white, 7, 5);
    set_bb19(board.white, 9, 5);
    CHECK(check_super4_19(t.lts4, board.white, board.black, 5, 5) == 1);
}

TEST_CASE("super4: black stones do not show on white-side query")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 3, 5);
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    set_bb19(board.black, 9, 5);
    CHECK(check_super4_19(t.lts4, board.white, board.black, 5, 5) == 0);
}
