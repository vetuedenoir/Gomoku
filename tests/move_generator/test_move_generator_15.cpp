#include "doctest.h"
#include "ai/MoveGenerator.hpp"
#include "game/board/GameBoard.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/helpers_15.hpp"

TEST_CASE("TEST_CASE [15x15] MoveGenerator: center stone radius=1 produces eight legal moves")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: center stone radius=1 produces eight legal moves" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 9, 9, CellStatus::White);

    MoveGenerator15 gen(1);
    auto moves = legalMovesList(gen, to_bb(b), Color::White);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::White, legalMovesMask);

    /*Bitboard 15*/
    CHECK(popcount_bb(legalMovesMask) == 8);
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    /*Moves*/
    CHECK(moves.size() == 8);
    CHECK_FALSE(contains_move(moves, 9, 9));

    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx != 0 || dy != 0)
                CHECK(contains_move(moves, 9 + dx, 9 + dy));
}

TEST_CASE("TEST_CASE [15x15] MoveGenerator: center stone radius=2 produces twenty-four legal moves")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: center stone radius=2 produces twenty-four legal moves" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 9, 9, CellStatus::Black);

    MoveGenerator15 gen(2);
    auto moves = legalMovesList(gen, to_bb(b), Color::White);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::White, legalMovesMask);
    
    /*Bitboard 15*/
    CHECK(popcount_bb(legalMovesMask) == 24);
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    /*Moves*/
    CHECK(moves.size() == 24);
    CHECK_FALSE(contains_move(moves, 9, 9));

    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx != 0 || dy != 0)
                CHECK(contains_move(moves, 9 + dx, 9 + dy));
}

TEST_CASE("TEST_CASE [15x15] MoveGenerator: corner stone radius=1 clips to board")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: corner stone radius=1 clips to board" << RESET << std::endl;
    
    GameBoard b = empty_board();
    place(b, 0, 0, CellStatus::Black);

    MoveGenerator15 gen(1);
    auto moves = legalMovesList(gen, to_bb(b), Color::White);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::White, legalMovesMask);

    /*Bitboard 15*/
    CHECK(popcount_bb(legalMovesMask) == 3);
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    /*Moves*/
    CHECK(moves.size() == 3);
    CHECK_FALSE(contains_move(moves, 0, 0));
    CHECK(contains_move(moves, 1, 0));
    CHECK(contains_move(moves, 0, 1));
    CHECK(contains_move(moves, 1, 1));
}

TEST_CASE("TEST_CASE [15x15] MoveGenerator: corner stone radius=2 clips to board")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: corner stone radius=2 clips to board" << RESET << std::endl;
    
    GameBoard b = empty_board();
    place(b, 0, 0, CellStatus::Black);

    MoveGenerator15 gen(2);
    auto moves = legalMovesList(gen, to_bb(b), Color::White);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::White, legalMovesMask);

    /*Bitboard 15*/
    CHECK(popcount_bb(legalMovesMask) == 8);
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    /*Moves*/
    CHECK(moves.size() == 8);
    CHECK_FALSE(contains_move(moves, 0, 0));
    CHECK(contains_move(moves, 2, 0));
    CHECK(contains_move(moves, 0, 2));
    CHECK(contains_move(moves, 2, 2));
    CHECK(contains_move(moves, 1, 2));
    CHECK(contains_move(moves, 2, 1));
}

TEST_CASE("TEST_CASE [15x15] MoveGenerator: adjacent occupied cells stay excluded")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: adjacent occupied cells stay excluded" << RESET << std::endl;
    
    GameBoard b = empty_board();
    place(b, 9,  9, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);


    MoveGenerator15 gen(1);
    auto moves = legalMovesList(gen, to_bb(b), Color::Black);
    t_BWBoard15 board = to_bb(b);

    /*Bitboard 15*/
    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);
    /*Moves*/
    CHECK(moves.size() == 10);
    CHECK_FALSE(contains_move(moves, 9, 9));
    CHECK_FALSE(contains_move(moves, 10, 9));
    CHECK(contains_move(moves, 8, 9));
    CHECK(contains_move(moves, 11, 9));
}

TEST_CASE("TEST_CASE [15x15] MoveGenerator: isLegalMove rejects out-of-board and occupied cells")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: isLegalMove rejects out-of-board and occupied cells" << RESET << std::endl;
    
    GameBoard b = empty_board();
    place(b, 5, 5, CellStatus::Black);

    MoveGenerator15 gen(1);
    t_BWBoard15 board = to_bb(b);

    CHECK(gen.isLegalMove(board, 6, 5, Color::White));
    CHECK_FALSE(gen.isLegalMove(board, 5, 5, Color::White));
    CHECK_FALSE(gen.isLegalMove(board, -1, 5, Color::White));
    CHECK_FALSE(gen.isLegalMove(board, 15, 5, Color::White));
}

// ── Double-three rule ─────────────────────────────────────────────────────────
//
// Same geometry as the 19x19 suite — coordinates fit well within 15x15 (0-14).
// Horizontal arm: (3,9)-(4,9), vertical arm: (5,7)-(5,8), candidate: (5,9).

TEST_CASE("TEST_CASE [15x15] MoveGenerator: generateMoves: double-three move is absent from result")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: generateMoves: double-three move is absent from result" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);
    place(b, 5, 7, CellStatus::Black);
    place(b, 5, 8, CellStatus::Black);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[double-three — (5,9) must be absent] Horizontal: (3,9)(4,9), Vertical: (5,7)(5,8)\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK_FALSE(contains_move(moves, 5, 9));  // double-three → filtered out
    CHECK(contains_move(moves, 3, 7));        // unambiguously legal neighbour → present
}

TEST_CASE("TEST_CASE [15x15] MoveGenerator: generateMoves: single free-three move is present in result")
{
    std::cout << TITLE_LINE << "TEST_CASE [15x15] MoveGenerator: generateMoves: single free-three move is present in result" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[single free-three — (5,9) must be present] Horizontal arm: (3,9)(4,9)\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK(contains_move(moves, 5, 9));   // single free-three → legal → present
    CHECK(contains_move(moves, 2, 9));   // another legal candidate in zone → present
}

TEST_CASE("[15x15] MoveGenerator: double-three is legal when move also captures")
{
    std::cout << TITLE_LINE << "[15x15] MoveGenerator: double-three is legal when move also captures" << RESET << std::endl;
    GameBoard b = empty_board();
    // Double-three partners
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);
    place(b, 5, 7, CellStatus::Black);
    place(b, 5, 8, CellStatus::Black);
    // Capture: playing B@(5,9) → (5,9)B (6,9)W (7,9)W (8,9)B
    place(b, 6, 9, CellStatus::White);
    place(b, 7, 9, CellStatus::White);
    place(b, 8, 9, CellStatus::Black);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[double-three + capture — (5,9) must be present]\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK(contains_move(moves, 5, 9));         // capture exempts double-three → present
    CHECK_FALSE(contains_move(moves, 6, 9));   // occupied by White
    CHECK_FALSE(contains_move(moves, 7, 9));   // occupied by White
    CHECK_FALSE(contains_move(moves, 8, 9));   // occupied by Black
}

// Edge case: vertical arm at top edge has a blocked end (off-board) → not a free-three.
// Only the horizontal arm qualifies → single free-three → candidate is legal.

TEST_CASE("[15x15] MoveGenerator: vertical arm at top edge is not a free-three — candidate is legal")
{
    std::cout << TITLE_LINE << "[15x15] MoveGenerator: vertical arm at top edge is not a free-three — candidate is legal" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 5, 0, CellStatus::Black);
    place(b, 5, 1, CellStatus::Black);
    // Horizontal arm
    place(b, 3, 2, CellStatus::Black);
    place(b, 4, 2, CellStatus::Black);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[top-edge vertical arm — (5,2) must be present]\n";
    std::cout << "  Vertical (5,0)-(5,1)-(5,2): top end off-board → blocked → not a free-three\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK(contains_move(moves, 5, 2));   // only horizontal free-three → legal
    CHECK(contains_move(moves, 3, 0));   // legal neighbour in zone → present
}

// True double-three in the interior: both arms fully open, no capture → absent.

TEST_CASE("[15x15] MoveGenerator: generateMoves: double-three without capture is absent from result")
{
    std::cout << TITLE_LINE << "[15x15] MoveGenerator: generateMoves: double-three without capture is absent from result" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 5, 2, CellStatus::Black);
    place(b, 5, 3, CellStatus::Black);
    place(b, 3, 4, CellStatus::Black);
    place(b, 4, 4, CellStatus::Black);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[double-three, no capture — (5,4) must be absent]\n";
    std::cout << "  Vertical: (5,2)(5,3), Horizontal: (3,4)(4,4), candidate: (5,4)\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK_FALSE(contains_move(moves, 5, 4));  // double-three, no capture → absent
    CHECK(contains_move(moves, 3, 2));        // unambiguously legal neighbour → present
}

// Same double-three but with a diagonal capture → exempt → present.
// Capture: B@(5,4) W@(6,5) W@(7,6) B@(8,7) — diagonal, does not block either arm.

TEST_CASE("[15x15] MoveGenerator: generateMoves: double-three with diagonal capture is present in result")
{
    std::cout << TITLE_LINE << "[15x15] MoveGenerator: generateMoves: double-three with diagonal capture is present in result" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 5, 2, CellStatus::Black);
    place(b, 5, 3, CellStatus::Black);
    place(b, 3, 4, CellStatus::Black);
    place(b, 4, 4, CellStatus::Black);
    place(b, 6, 5, CellStatus::White);
    place(b, 7, 6, CellStatus::White);
    place(b, 8, 7, CellStatus::Black);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[double-three + diagonal capture — (5,4) must be present]\n";
    std::cout << "  Capture: B@(5,4) W@(6,5) W@(7,6) B@(8,7)\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK(contains_move(moves, 5, 4));  // capture exempts double-three → present
}

// Double-three near the right edge with a lone White on the diagonal — no valid capture.
// Horizontal arm: (11,7)-(12,7), vertical arm: (13,5)-(13,6), candidate: (13,7).
// Both arm ends open. Lone W@(14,8) on diagonal, no second white and no anchor
// → detect_captures = false → double-three stands → (13,7) must be absent.

TEST_CASE("[15x15] MoveGenerator: double-three with insufficient capture (lone white, no anchor) is absent")
{
    std::cout << TITLE_LINE << "[15x15] MoveGenerator: double-three with insufficient capture (lone white, no anchor) is absent" << RESET << std::endl;
    GameBoard b = empty_board();
    place(b, 11, 7, CellStatus::Black);
    place(b, 12, 7, CellStatus::Black);
    place(b, 13, 5, CellStatus::Black);
    place(b, 13, 6, CellStatus::Black);
    // Lone white diagonally adjacent — no B-W-W-B pattern possible
    place(b, 14, 8, CellStatus::White);

    MoveGenerator15 gen(2);
    t_BWBoard15 board = to_bb(b);

    bitboard15 legalMovesMask = {};
    gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
    auto moves = legalMovesList(gen, board, Color::Black);

    std::cout << "\n[double-three, lone W@(14,8) — no anchor → no capture] Candidate: (13,7) Black\n";
    print_bb_overlay<BoardTraits<15>>(board, legalMovesMask);

    CHECK_FALSE(contains_move(moves, 13, 7));  // double-three, no valid capture → absent
    CHECK(contains_move(moves, 11, 5));        // unambiguously legal neighbour → present
}
