#include "doctest.h"
#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"

using Traits19 = BoardTraits<19>;
using BB19     = Traits19::Bitboard;

static t_BWBoard19 make_board() { t_BWBoard19 b = {}; return b; }

TEST_CASE("detect_captures: no capture when pattern is incomplete")
{
    t_BWBoard19 board = make_board();
    set_bb_generic<Traits19>(board.black, 5, 5);
    set_bb_generic<Traits19>(board.white, 6, 5);

    BB19 captured = {};

    CHECK(detect_captures<Traits19>(board, 5, 5, Color::Black, captured) == false);
    CHECK(popcount_bb_generic<Traits19>(captured) == 0);
}

TEST_CASE("detect_captures: horizontal — placed stone on the left")
{
    // B[placed] W W B  →  captures the two whites
    t_BWBoard19 board = make_board();
    set_bb_generic<Traits19>(board.black, 2, 5);
    set_bb_generic<Traits19>(board.white, 3, 5);
    set_bb_generic<Traits19>(board.white, 4, 5);
    set_bb_generic<Traits19>(board.black, 5, 5);

    BB19 captured = {};
    detect_captures<Traits19>(board, 2, 5, Color::Black, captured);

    CHECK(popcount_bb_generic<Traits19>(captured) == 2);
    CHECK(get_bb_generic<Traits19>(captured, 3, 5));
    CHECK(get_bb_generic<Traits19>(captured, 4, 5));
}

TEST_CASE("detect_captures: horizontal — placed stone on the right")
{
    // B W W B[placed]  →  captures the two whites
    t_BWBoard19 board = make_board();
    set_bb_generic<Traits19>(board.black, 2, 5);
    set_bb_generic<Traits19>(board.white, 3, 5);
    set_bb_generic<Traits19>(board.white, 4, 5);
    set_bb_generic<Traits19>(board.black, 5, 5);

    BB19 captured = {};
    detect_captures<Traits19>(board, 5, 5, Color::Black, captured);

    CHECK(popcount_bb_generic<Traits19>(captured) == 2);
    CHECK(get_bb_generic<Traits19>(captured, 3, 5));
    CHECK(get_bb_generic<Traits19>(captured, 4, 5));
}

TEST_CASE("detect_captures: vertical capture")
{
    // Column 5, rows 1-4: B[placed] W W B
    t_BWBoard19 board = make_board();
    set_bb_generic<Traits19>(board.black, 5, 1);
    set_bb_generic<Traits19>(board.white, 5, 2);
    set_bb_generic<Traits19>(board.white, 5, 3);
    set_bb_generic<Traits19>(board.black, 5, 4);

    BB19 captured = {};
    detect_captures<Traits19>(board, 5, 1, Color::Black, captured);

    CHECK(popcount_bb_generic<Traits19>(captured) == 2);
    CHECK(get_bb_generic<Traits19>(captured, 5, 2));
    CHECK(get_bb_generic<Traits19>(captured, 5, 3));
}

TEST_CASE("detect_captures: diagonal capture")
{
    // Main diagonal \: B[placed](2,2)  W(3,3)  W(4,4)  B(5,5)
    t_BWBoard19 board = make_board();
    set_bb_generic<Traits19>(board.black, 2, 2);
    set_bb_generic<Traits19>(board.white, 3, 3);
    set_bb_generic<Traits19>(board.white, 4, 4);
    set_bb_generic<Traits19>(board.black, 5, 5);

    BB19 captured = {};
    detect_captures<Traits19>(board, 2, 2, Color::Black, captured);

    CHECK(popcount_bb_generic<Traits19>(captured) == 2);
    CHECK(get_bb_generic<Traits19>(captured, 3, 3));
    CHECK(get_bb_generic<Traits19>(captured, 4, 4));
}

TEST_CASE("detect_captures: two simultaneous captures in different directions")
{
    t_BWBoard19 board = make_board();

    // Horizontal: B W W B[placed]
    set_bb_generic<Traits19>(board.black, 2, 5);
    set_bb_generic<Traits19>(board.white, 3, 5);
    set_bb_generic<Traits19>(board.white, 4, 5);

    // Vertical: B W W B[placed] (same placed stone at (5,5))
    set_bb_generic<Traits19>(board.black, 5, 2);
    set_bb_generic<Traits19>(board.white, 5, 3);
    set_bb_generic<Traits19>(board.white, 5, 4);

    set_bb_generic<Traits19>(board.black, 5, 5);

    BB19 captured = {};
    detect_captures<Traits19>(board, 5, 5, Color::Black, captured);

    CHECK(popcount_bb_generic<Traits19>(captured) == 4);
}

TEST_CASE("apply_captures removes victim stones and leaves attacker untouched")
{
    t_BWBoard19 board = make_board();
    set_bb_generic<Traits19>(board.black, 2, 5);
    set_bb_generic<Traits19>(board.white, 3, 5);
    set_bb_generic<Traits19>(board.white, 4, 5);
    set_bb_generic<Traits19>(board.black, 5, 5);

    BB19 captured = {};
    detect_captures<Traits19>(board, 5, 5, Color::Black, captured);
    apply_captures<Traits19>(board, captured, Color::Black);

    CHECK(!get_bb_generic<Traits19>(board.white, 3, 5));
    CHECK(!get_bb_generic<Traits19>(board.white, 4, 5));
    CHECK( get_bb_generic<Traits19>(board.black, 2, 5));
    CHECK( get_bb_generic<Traits19>(board.black, 5, 5));
}
