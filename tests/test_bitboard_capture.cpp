#include "doctest.h"
#include "bitboard.hpp"
#include "logger/Logger.hpp"

static t_BWBoard19 make_board() { t_BWBoard19 b = {}; return b; }

TEST_CASE("detect_captures: no capture when pattern is incomplete")
{
    t_BWBoard19 board = make_board();
    set(board.black, 5, 5);
    set(board.white, 6, 5);

    bitboard19 captured = {};
    
    CHECK(detect_captures(board, 5, 5, Color::Black, captured) == false);
    CHECK(popcount_bb(captured) == 0);
}

TEST_CASE("detect_captures: horizontal — placed stone on the left")
{
    // B[placed] W W B  →  captures the two whites
    t_BWBoard19 board = make_board();
    set(board.black, 2, 5);
    set(board.white, 3, 5);
    set(board.white, 4, 5);
    set(board.black, 5, 5);

    bitboard19 captured = {};
    detect_captures(board, 2, 5, Color::Black, captured);

    CHECK(popcount_bb(captured) == 2);
    CHECK(get(captured, index(3, 5)));
    CHECK(get(captured, index(4, 5)));
}

TEST_CASE("detect_captures: horizontal — placed stone on the right")
{
    // B W W B[placed]  →  captures the two whites
    t_BWBoard19 board = make_board();
    set(board.black, 2, 5);
    set(board.white, 3, 5);
    set(board.white, 4, 5);
    set(board.black, 5, 5);

    bitboard19 captured = {};
    detect_captures(board, 5, 5, Color::Black, captured);

    CHECK(popcount_bb(captured) == 2);
    CHECK(get(captured, index(3, 5)));
    CHECK(get(captured, index(4, 5)));
}

TEST_CASE("detect_captures: vertical capture")
{
    // Column 5, rows 1-4: B[placed] W W B
    t_BWBoard19 board = make_board();
    set(board.black, 5, 1);
    set(board.white, 5, 2);
    set(board.white, 5, 3);
    set(board.black, 5, 4);

    bitboard19 captured = {};
    detect_captures(board, 5, 1, Color::Black, captured);

    CHECK(popcount_bb(captured) == 2);
    CHECK(get(captured, index(5, 2)));
    CHECK(get(captured, index(5, 3)));
}

TEST_CASE("detect_captures: diagonal capture")
{
    // Main diagonal \: B[placed](2,2)  W(3,3)  W(4,4)  B(5,5)
    t_BWBoard19 board = make_board();
    set(board.black, 2, 2);
    set(board.white, 3, 3);
    set(board.white, 4, 4);
    set(board.black, 5, 5);

    bitboard19 captured = {};
    detect_captures(board, 2, 2, Color::Black, captured);

    CHECK(popcount_bb(captured) == 2);
    CHECK(get(captured, index(3, 3)));
    CHECK(get(captured, index(4, 4)));
}

TEST_CASE("detect_captures: two simultaneous captures in different directions")
{
    t_BWBoard19 board = make_board();

    // Horizontal: B W W B[placed]
    set(board.black, 2, 5);
    set(board.white, 3, 5);
    set(board.white, 4, 5);

    // Vertical: B W W B[placed] (same placed stone at (5,5))
    set(board.black, 5, 2);
    set(board.white, 5, 3);
    set(board.white, 5, 4);

    set(board.black, 5, 5);

    bitboard19 captured = {};
    detect_captures(board, 5, 5, Color::Black, captured);

    CHECK(popcount_bb(captured) == 4);
}

TEST_CASE("apply_captures removes victim stones and leaves attacker untouched")
{
    t_BWBoard19 board = make_board();
    set(board.black, 2, 5);
    set(board.white, 3, 5);
    set(board.white, 4, 5);
    set(board.black, 5, 5);

    bitboard19 captured = {};
    detect_captures(board, 5, 5, Color::Black, captured);
    apply_captures(board, captured, Color::Black);

    CHECK(!get(board.white, index(3, 5)));
    CHECK(!get(board.white, index(4, 5)));
    CHECK( get(board.black, index(2, 5)));
    CHECK( get(board.black, index(5, 5)));
}
