#include "doctest.h"
#include "pattern_fixture.hpp"

// =============================================================================
// is_Open_4 — open-four detection
//
// Returns:
//   2  — fully open (free space on both ends)
//   1  — partially open (one end blocked by opponent or board edge)
//   0  — no four-in-a-row, or both ends blocked
//
// Key: the board edge itself is treated as "blocked" (opposant_left == -1
// or opposant_right == -1 decrements the score just like an opponent stone).
// =============================================================================

// ─── Empty / trivial ─────────────────────────────────────────────────────────

TEST_CASE("open4: empty board returns 0")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 9, 9) == 0);
}

TEST_CASE("open4: 3 in a row does not trigger open-four")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.black, 5, 5);
    set_bb19(board.black, 6, 5);
    set_bb19(board.black, 7, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 6, 5) == 0);
}

// ─── Horizontal ──────────────────────────────────────────────────────────────

TEST_CASE("open4: horizontal — fully open (both ends free)")
{
    // [ ] B B B B [ ]
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 2);
}

TEST_CASE("open4: horizontal — left end blocked by opponent")
{
    // W B B B B [ ]
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.white, 2, 5);
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 1);
}

TEST_CASE("open4: horizontal — right end blocked by opponent")
{
    // [ ] B B B B W
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    set_bb19(board.white, 7, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 1);
}

TEST_CASE("open4: horizontal — both ends blocked by opponent")
{
    // W B B B B W
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.white, 2, 5);
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    set_bb19(board.white, 7, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 0);
}

TEST_CASE("open4: horizontal — 4 stones starting at x=0 (left edge blocks)")
{
    // [edge] B B B B [ ]   — left edge counts as a block
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 0; x <= 3; x++)
        set_bb19(board.black, x, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 1, 5) == 1);
}

TEST_CASE("open4: horizontal — 4 stones ending at x=18 (right edge blocks)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 15; x <= 18; x++)
        set_bb19(board.black, x, 5);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 16, 5) == 1);
}

// ─── Vertical ────────────────────────────────────────────────────────────────

TEST_CASE("open4: vertical — fully open")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int y = 3; y <= 6; y++)
        set_bb19(board.black, 5, y);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 2);
}

TEST_CASE("open4: vertical — top end blocked by opponent")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.white, 5, 2);
    for (int y = 3; y <= 6; y++)
        set_bb19(board.black, 5, y);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 1);
}

TEST_CASE("open4: vertical — bottom end blocked by opponent")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int y = 3; y <= 6; y++)
        set_bb19(board.black, 5, y);
    set_bb19(board.white, 5, 7);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 5) == 1);
}

TEST_CASE("open4: vertical — 4 stones starting at y=0 (top edge blocks)")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int y = 0; y <= 3; y++)
        set_bb19(board.black, 5, y);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 1) == 1);
}

// ─── Diagonal backslash (\) ───────────────────────────────────────────────────

TEST_CASE("open4: diagonal-backslash — fully open")
{
    // (3,3) (4,4) (5,5) (6,6)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int i = 0; i < 4; i++)
        set_bb19(board.black, 3 + i, 3 + i);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 4, 4) == 2);
}

TEST_CASE("open4: diagonal-backslash — one end blocked")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    set_bb19(board.white, 2, 2);
    for (int i = 0; i < 4; i++)
        set_bb19(board.black, 3 + i, 3 + i);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 4, 4) == 1);
}

// ─── Diagonal slash (/) ──────────────────────────────────────────────────────

TEST_CASE("open4: diagonal-slash — fully open")
{
    // (6,3) (5,4) (4,5) (3,6)
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int i = 0; i < 4; i++)
        set_bb19(board.black, 6 - i, 3 + i);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 4) == 2);
}

TEST_CASE("open4: diagonal-slash — one end blocked")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int i = 0; i < 4; i++)
        set_bb19(board.black, 6 - i, 3 + i);
    set_bb19(board.white, 2, 7);
    CHECK(is_Open_4_19(t.lt4, board.black, board.white, 5, 4) == 1);
}

// ─── Color isolation ─────────────────────────────────────────────────────────

TEST_CASE("open4: white fully open four is detected on white board")
{
    // [ ] W W W W [ ]
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 6; x++)
        set_bb19(board.white, x, 5);
    CHECK(is_Open_4_19(t.lt4, board.white, board.black, 5, 5) == 2);
}

TEST_CASE("open4: black four does not show on white-side query")
{
    PatternTables& t = PatternTables::get();
    t_BWBoard19 board = empty_bb();
    for (int x = 3; x <= 6; x++)
        set_bb19(board.black, x, 5);
    CHECK(is_Open_4_19(t.lt4, board.white, board.black, 5, 5) == 0);
}
