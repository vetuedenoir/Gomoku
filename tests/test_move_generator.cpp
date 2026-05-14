#include "doctest.h"
#include "ai/MoveGenerator.hpp"
#include "game/GameBoard.hpp"
#include "bitboard/bitboard.hpp"

static GameBoard empty_board()
{
    return GameBoard(19, Seat::First);
}

static void place(GameBoard& b, int col, int row, CellStatus color)
{
    b.placeStoneOfColor(col, row, color);
}

static t_BWBoard19 to_bb(const GameBoard& b)
{
    return GameBoard_to_bitboard(b);
}

TEST_CASE("MoveGenerator: empty board has no active-zone legal moves")
{
    MoveGenerator gen(1);
    bitboard19 legalMoves = {};

    gen.generateLegalMoves(to_bb(empty_board()), Color::Black, legalMoves);

    CHECK(popcount_bb(legalMoves) == 0);
}

TEST_CASE("MoveGenerator: center stone radius=1 produces eight legal moves")
{
    GameBoard b = empty_board();
    place(b, 9, 9, CellStatus::Black);

    MoveGenerator gen(1);
    bitboard19 legalMoves = {};

    gen.generateLegalMoves(to_bb(b), Color::White, legalMoves);

    CHECK(popcount_bb(legalMoves) == 8);
    CHECK_FALSE(get_bb19(legalMoves, index_bb19(9, 9)));

    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx != 0 || dy != 0)
                CHECK(get_bb19(legalMoves, index_bb19(9 + dx, 9 + dy)));
}

TEST_CASE("MoveGenerator: corner stone radius=1 clips to board")
{
    GameBoard b = empty_board();
    place(b, 0, 0, CellStatus::Black);

    MoveGenerator gen(1);
    bitboard19 legalMoves = {};

    gen.generateLegalMoves(to_bb(b), Color::White, legalMoves);

    CHECK(popcount_bb(legalMoves) == 3);
    CHECK(get_bb19(legalMoves, index_bb19(1, 0)));
    CHECK(get_bb19(legalMoves, index_bb19(0, 1)));
    CHECK(get_bb19(legalMoves, index_bb19(1, 1)));
    CHECK_FALSE(get_bb19(legalMoves, index_bb19(0, 0)));
}

TEST_CASE("MoveGenerator: adjacent occupied cells stay excluded")
{
    GameBoard b = empty_board();
    place(b, 9,  9, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);

    MoveGenerator gen(1);
    bitboard19 legalMoves = {};

    gen.generateLegalMoves(to_bb(b), Color::Black, legalMoves);

    CHECK(popcount_bb(legalMoves) == 10);
    CHECK_FALSE(get_bb19(legalMoves, index_bb19(9, 9)));
    CHECK_FALSE(get_bb19(legalMoves, index_bb19(10, 9)));
    CHECK(get_bb19(legalMoves, index_bb19(8, 9)));
    CHECK(get_bb19(legalMoves, index_bb19(11, 9)));
}

TEST_CASE("MoveGenerator: isLegalMove rejects out-of-board and occupied cells")
{
    GameBoard b = empty_board();
    place(b, 5, 5, CellStatus::Black);

    MoveGenerator gen(1);
    t_BWBoard19 board = to_bb(b);

    CHECK(gen.isLegalMove(board, 6, 5, Color::White));
    CHECK_FALSE(gen.isLegalMove(board, 5, 5, Color::White));
    CHECK_FALSE(gen.isLegalMove(board, -1, 5, Color::White));
    CHECK_FALSE(gen.isLegalMove(board, 19, 5, Color::White));
}
