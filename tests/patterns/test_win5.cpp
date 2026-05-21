#include "doctest.h"
#include "pattern_fixture.hpp"

// =============================================================================
// isWin_ultra — five-in-a-row detection
//
// Returns 1 if at least one 5-in-a-row pattern containing position (x,y)
// is fully matched on the board, 0 otherwise.
// =============================================================================

// ─── Empty / trivial ─────────────────────────────────────────────────────────

TEST_CASE("win5: empty board returns 0")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    CHECK(isWin_ultra_19(t.lt5, board.black, 9, 9) == 0);
}

TEST_CASE("win5: 4 in a row is not a win")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) == 0);
}

// ─── Horizontal wins ─────────────────────────────────────────────────────────

TEST_CASE("win5: 5 horizontal — detected from every stone position")
{
    // B B B B B  at y=5, x in [3..7]
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 7; x++)
        set_bb19(board.black, x, 5);

    SUBCASE("query from x=3") { CHECK(isWin_ultra_19(t.lt5, board.black, 3, 5) != 0); }
    SUBCASE("query from x=4") { CHECK(isWin_ultra_19(t.lt5, board.black, 4, 5) != 0); }
    SUBCASE("query from x=5") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) != 0); }
    SUBCASE("query from x=6") { CHECK(isWin_ultra_19(t.lt5, board.black, 6, 5) != 0); }
    SUBCASE("query from x=7") { CHECK(isWin_ultra_19(t.lt5, board.black, 7, 5) != 0); }
}

TEST_CASE("win5: 5 at left board edge (x=0..4)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 0; x <= 4; x++)
        set_bb19(board.black, x, 5);
    CHECK(isWin_ultra_19(t.lt5, board.black, 2, 5) != 0);
}

TEST_CASE("win5: 5 at right board edge (x=14..18)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 14; x <= 18; x++)
        set_bb19(board.black, x, 5);
    CHECK(isWin_ultra_19(t.lt5, board.black, 16, 5) != 0);
}

TEST_CASE("win5: 6 in a row still registers as win (sub-sequence of 5 matches)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 8; x++)
        set_bb19(board.black, x, 5);
    CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) != 0);
}

// ─── Vertical wins ───────────────────────────────────────────────────────────

TEST_CASE("win5: 5 vertical — detected from every stone position")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int y = 3; y <= 7; y++)
        set_bb19(board.black, 5, y);

    SUBCASE("query from y=3") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 3) != 0); }
    SUBCASE("query from y=4") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 4) != 0); }
    SUBCASE("query from y=5") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) != 0); }
    SUBCASE("query from y=6") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 6) != 0); }
    SUBCASE("query from y=7") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 7) != 0); }
}

TEST_CASE("win5: 5 at top board edge (y=0..4)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int y = 0; y <= 4; y++)
        set_bb19(board.black, 5, y);
    CHECK(isWin_ultra_19(t.lt5, board.black, 5, 2) != 0);
}

TEST_CASE("win5: 5 at bottom board edge (y=14..18)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int y = 14; y <= 18; y++)
        set_bb19(board.black, 5, y);
    CHECK(isWin_ultra_19(t.lt5, board.black, 5, 16) != 0);
}

// ─── Diagonal wins ───────────────────────────────────────────────────────────

TEST_CASE("win5: 5 diagonal-backslash (\\)")
{
    // (3,3) (4,4) (5,5) (6,6) (7,7)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int i = 0; i < 5; i++)
        set_bb19(board.black, 3 + i, 3 + i);

    SUBCASE("query from start")  { CHECK(isWin_ultra_19(t.lt5, board.black, 3, 3) != 0); }
    SUBCASE("query from middle") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) != 0); }
    SUBCASE("query from end")    { CHECK(isWin_ultra_19(t.lt5, board.black, 7, 7) != 0); }
}

TEST_CASE("win5: 5 diagonal-slash (/)")
{
    // (7,3) (6,4) (5,5) (4,6) (3,7)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int i = 0; i < 5; i++)
        set_bb19(board.black, 7 - i, 3 + i);

    SUBCASE("query from start")  { CHECK(isWin_ultra_19(t.lt5, board.black, 7, 3) != 0); }
    SUBCASE("query from middle") { CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) != 0); }
    SUBCASE("query from end")    { CHECK(isWin_ultra_19(t.lt5, board.black, 3, 7) != 0); }
}

// ─── Color isolation ─────────────────────────────────────────────────────────

TEST_CASE("win5: black stones do not trigger white-side win")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 7; x++)
        set_bb19(board.black, x, 5);
    CHECK(isWin_ultra_19(t.lt5, board.white, 5, 5) == 0);
}

TEST_CASE("win5: white 5 in a row is detected on white board")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 7; x++)
        set_bb19(board.white, x, 5);
    CHECK(isWin_ultra_19(t.lt5, board.white, 5, 5) != 0);
}

TEST_CASE("win5: mixed stones (4 black + 1 white) are not a black win")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    set_bb19(board.white, 7, 5);  // interrupts the run
    CHECK(isWin_ultra_19(t.lt5, board.black, 5, 5) == 0);
}
