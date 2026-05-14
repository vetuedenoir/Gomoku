#include "doctest.h"
#include "ai/MoveGenerator.hpp"
#include "game/GameBoard.hpp"
#include "bitboard/bitboard.hpp"
#include <iostream>

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

// ── Double-three rule ─────────────────────────────────────────────────────────
//
// Setup: place Black stones so that playing at (5, 9) simultaneously creates
// a horizontal free-three  (3,9)–(4,9)–(5,9)  and
// a vertical   free-three  (5,7)–(5,8)–(5,9).
// Both lines have open ends → the move is a double-three and must be illegal.

TEST_CASE("isLegalMove: double-three is illegal")
{
    GameBoard b = empty_board();
    // Horizontal three partner stones
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);
    // Vertical three partner stones
    place(b, 5, 7, CellStatus::Black);
    place(b, 5, 8, CellStatus::Black);

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[double-three illegal] Board before move — candidate: (5,9) Black\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 5, 9, Color::Black);
    std::cout << "  isLegalMove(5,9,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: ILLEGAL)\n";

    CHECK_FALSE(result);
}

TEST_CASE("isLegalMove: single free-three is legal")
{
    GameBoard b = empty_board();
    // Only one free-three arm (horizontal): (3,9) (4,9) → play at (5,9)
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[single free-three legal] Board before move — candidate: (5,9) Black\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 5, 9, Color::Black);
    std::cout << "  isLegalMove(5,9,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: LEGAL)\n";

    CHECK(result);
}

TEST_CASE("isLegalMove: double-three is legal when move also captures")
{
    // Same double-three setup as above PLUS a B W W B capture line along row 9:
    //   Black@(8,9) · White@(6,9) · White@(7,9) ← playing Black at (5,9) captures them
    GameBoard b = empty_board();
    // Double-three partners
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);
    place(b, 5, 7, CellStatus::Black);
    place(b, 5, 8, CellStatus::Black);
    // Capture setup: playing at (5,9) → (5,9)B (6,9)W (7,9)W (8,9)B
    place(b, 6, 9, CellStatus::White);
    place(b, 7, 9, CellStatus::White);
    place(b, 8, 9, CellStatus::Black);

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[double-three + capture legal] Board before move — candidate: (5,9) Black\n";
    std::cout << "  Capture pattern on row 9: (5,9)[play] W@(6,9) W@(7,9) B@(8,9)\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 5, 9, Color::Black);
    std::cout << "  isLegalMove(5,9,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: LEGAL — capture exempts double-three)\n";

    CHECK(result);
}

// ── Edge cases ────────────────────────────────────────────────────────────────

// Case 1: vertical arm starts at row 0 (top edge).
// The three (5,0)-(5,1)-(5,2) has its top neighbour off-board → score=256,
// which passes the >256 filter and is still counted as a free three.
// Combined with a horizontal arm (3,2)-(4,2)-(5,2): double-three → illegal.
TEST_CASE("isLegalMove: double-three with vertical arm touching top edge is illegal")
{
    GameBoard b = empty_board();
    place(b, 5, 0, CellStatus::Black);
    place(b, 5, 1, CellStatus::Black);
    // Horizontal arm
    place(b, 3, 2, CellStatus::Black);
    place(b, 4, 2, CellStatus::Black);

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[top-edge double-three illegal] Vertical arm (5,0)-(5,1)-(5,2), horizontal (3,2)-(4,2)-(5,2)\n";
    std::cout << "  Top edge: oposant_pos[1]=-1 → score=256, still a free three.\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 5, 2, Color::Black);
    std::cout << "  isLegalMove(5,2,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: ILLEGAL)\n";

    CHECK_FALSE(result);
}

// Case 2: same near-top double-three, but the move also captures → legal.
// Capture: (5,2)B (6,2)W (7,2)W (8,2)B along row 2.
TEST_CASE("isLegalMove: near-top double-three is legal when move captures")
{
    GameBoard b = empty_board();
    place(b, 5, 0, CellStatus::Black);
    place(b, 5, 1, CellStatus::Black);
    place(b, 3, 2, CellStatus::Black);
    place(b, 4, 2, CellStatus::Black);
    // Capture
    place(b, 6, 2, CellStatus::White);
    place(b, 7, 2, CellStatus::White);
    place(b, 8, 2, CellStatus::Black);

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[top-edge double-three + capture legal] Candidate: (5,2) Black\n";
    std::cout << "  Capture on row 2: (5,2)[play] W@(6,2) W@(7,2) B@(8,2)\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 5, 2, Color::Black);
    std::cout << "  isLegalMove(5,2,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: LEGAL)\n";

    CHECK(result);
}

// Case 3: opponent closes one end completely (immediate + far neighbour blocked)
// → score left = 256+128 = 384 > 256 → horizontal arm filtered out.
// Only the vertical arm survives: single free-three → legal.
TEST_CASE("isLegalMove: opponent double-blocks one arm leaving single free-three")
{
    GameBoard b = empty_board();
    // Horizontal arm
    place(b, 3, 9, CellStatus::Black);
    place(b, 4, 9, CellStatus::Black);
    // Close the left end tightly: W@(2,9) is the immediate left; W@(1,9) is the far left.
    // score_left = 256 (immediate) + whatever the check produces. Actually since
    // oposant_pos[1] (immediate) fires → score_left = 256, then oposant_pos[2] (immediate
    // right, col 6) is empty → score_right = 0. Total = 256 (not > 256)!
    // We need BOTH sides to score > 0 summing above 256.
    // Block the right immediate too: W@(6,9) gives score_right = 256. Total = 512 > 256 → filtered.
    place(b, 2, 9, CellStatus::White);  // immediate left
    place(b, 6, 9, CellStatus::White);  // immediate right
    // Vertical arm stays fully open
    place(b, 5, 7, CellStatus::Black);
    place(b, 5, 8, CellStatus::Black);

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[arm closed by opponent on both sides] Horizontal arm blocked, vertical arm free\n";
    std::cout << "  W@(2,9) closes left, W@(6,9) closes right → horizontal not a free-three.\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 5, 9, Color::Black);
    std::cout << "  isLegalMove(5,9,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: LEGAL — only one free-three)\n";

    CHECK(result);
}

// Case 4: insufficient capture — only ONE white between the played stone and the
// right edge (cols 17-18).  Gomoku capture requires exactly two opponent stones
// flanked by two of your own: B W W B.  With only one white there is no anchor
// beyond it inside the board → detect_captures returns false → double-three
// stands → illegal.
TEST_CASE("isLegalMove: double-three with only one-white near edge is still illegal")
{
    GameBoard b = empty_board();
    // Double-three at (16,9)
    place(b, 14, 9, CellStatus::Black);
    place(b, 15, 9, CellStatus::Black);
    place(b, 16, 7, CellStatus::Black);
    place(b, 16, 8, CellStatus::Black);
    // Only one white to the right before the edge — not a valid capture
    place(b, 17, 9, CellStatus::White);
    // No black anchor at col 19 (off-board) → capture impossible

    MoveGenerator gen(2);
    t_BWBoard19 board = to_bb(b);

    std::cout << "\n[one-white near edge — no capture] Candidate: (16,9) Black\n";
    std::cout << "  W@(17,9) only — no anchor at (19,9) → detect_captures returns false.\n";
    print_bb_19_colored(board);
    bool result = gen.isLegalMove(board, 16, 9, Color::Black);
    std::cout << "  isLegalMove(16,9,Black) = " << (result ? "LEGAL" : "ILLEGAL") << " (expected: ILLEGAL)\n";

    CHECK_FALSE(result);
}
