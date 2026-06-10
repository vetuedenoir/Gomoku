#include "doctest.h"
#include "game/board/GameBoard.hpp"
#include "ai/ActiveZone.hpp"
#include "game/board/DebugBoard.hpp"
#include "bitboard/bitboard.hpp"

static GameBoard empty_board()
{
    return GameBoard(19, Color::Black);
}

static void place(GameBoard& b, int col, int row, CellStatus color)
{
    b.placeStoneOfColor(col, row, color);
}

static t_BWBoard19 to_bb(const GameBoard& b)
{
    return GameBoard_to_bitboard<BoardTraits<19>>(b);
}

TEST_CASE("ActiveZone: empty board yields zero candidates")
{
    ActiveZone19 zone(2);
    zone.initialize(to_bb(empty_board()));

    CHECK(zone.size() == 0);
}

TEST_CASE("ActiveZone: corner stone clips to board boundary")
{
    GameBoard b = empty_board();
    place(b, 0, 0, CellStatus::Black);

    ActiveZone19 zone(1);
    zone.initialize(to_bb(b));

    // only 3 in-board neighbors: (1,0), (0,1), (1,1)
    CHECK(zone.size() == 3);
    CHECK(zone.contains(1, 0));
    CHECK(zone.contains(0, 1));
    CHECK(zone.contains(1, 1));

    // stone itself must never be a candidate
    CHECK_FALSE(zone.contains(0, 0));

    // cells beyond the corner are simply absent
    CHECK_FALSE(zone.contains(2, 0));
    CHECK_FALSE(zone.contains(0, 2));
}

TEST_CASE("ActiveZone: center stone radius=1 yields exactly 8 candidates")
{
    GameBoard b = empty_board();
    place(b, 9, 9, CellStatus::Black);

    ActiveZone19 zone(1);
    zone.initialize(to_bb(b));

    CHECK(zone.size() == 8);
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx != 0 || dy != 0)
                CHECK(zone.contains(9 + dx, 9 + dy));

    CHECK_FALSE(zone.contains(9, 9));
}

TEST_CASE("ActiveZone: center stone radius=2 yields exactly 24 candidates")
{
    GameBoard b = empty_board();
    place(b, 9, 9, CellStatus::Black);

    ActiveZone19 zone(2);
    zone.initialize(to_bb(b));

    // 5x5 window minus the occupied center = 24
    CHECK(zone.size() == 24);
    CHECK_FALSE(zone.contains(9, 9));
    CHECK_FALSE(zone.contains(6, 9)); // just outside radius
    CHECK_FALSE(zone.contains(9, 12));
}

TEST_CASE("ActiveZone: adjacent stones merge zones correctly")
{
    GameBoard b = empty_board();
    place(b, 9,  9, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);

    ActiveZone19 zone(1);
    zone.initialize(to_bb(b));

    // 3x4 bounding box minus 2 occupied stones = 10 candidates
    CHECK(zone.size() == 10);

    // shared neighbor on top
    CHECK(zone.contains(9,  8));
    CHECK(zone.contains(10, 8));

    // flanks
    CHECK(zone.contains(8, 9));
    CHECK(zone.contains(11, 9));

    // occupied cells excluded
    CHECK_FALSE(zone.contains(9,  9));
    CHECK_FALSE(zone.contains(10, 9));
}

TEST_CASE("ActiveZone: edge stone clips out-of-board neighbors")
{
    GameBoard b = empty_board();
    place(b, 0, 9, CellStatus::Black); // left edge, mid-row

    ActiveZone19 zone(1);
    zone.initialize(to_bb(b));

    // in-board neighbors: (0,8),(1,8),(1,9),(0,10),(1,10)
    CHECK(zone.size() == 5);
    CHECK(zone.contains(0, 8));
    CHECK(zone.contains(1, 8));
    CHECK(zone.contains(1, 9));
    CHECK(zone.contains(0, 10));
    CHECK(zone.contains(1, 10));

    CHECK_FALSE(zone.contains(0, 9)); // occupied
}

TEST_CASE("ActiveZone: clear() resets all candidates")
{
    GameBoard b = empty_board();
    place(b, 9, 9, CellStatus::Black);

    ActiveZone19 zone(2);
    zone.initialize(to_bb(b));

    CHECK(zone.size() == 24);

    zone.clear();

    CHECK(zone.size() == 0);
    CHECK_FALSE(zone.contains(8, 9));
    CHECK_FALSE(zone.contains(10, 9));

    printBoardWithOverlay(b, buildOverlayFromActiveZone(zone));
}

TEST_CASE("ActiveZone: re-initialize overwrites previous state")
{
    ActiveZone19 zone(1);

    GameBoard b1 = empty_board();
    place(b1, 5, 5, CellStatus::Black);
    zone.initialize(to_bb(b1));

    CHECK(zone.contains(4, 5));
    CHECK_FALSE(zone.contains(9, 9));

    GameBoard b2 = empty_board();
    place(b2, 9, 9, CellStatus::White);
    zone.initialize(to_bb(b2));

    // old neighbors gone
    CHECK_FALSE(zone.contains(4, 5));
    // new neighbors present
    CHECK(zone.contains(9, 8));
    CHECK(zone.contains(10, 9));
}

TEST_CASE("ActiveZone: visual overlay — neighbors of all stones")
{
    GameBoard b = empty_board();

    place(b, 1,  1, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);
    place(b, 11,  9, CellStatus::Black);
    place(b, 1, 16, CellStatus::Black);
    place(b, 18, 2, CellStatus::White);

    ActiveZone19 zone(2);
    zone.initialize(to_bb(b));

    CHECK(zone.contains(0, 0));
    CHECK(zone.contains(2, 2));
    CHECK(zone.contains(9, 8));
    CHECK_FALSE(zone.contains(1,  1));
    CHECK_FALSE(zone.contains(10, 9));
}
