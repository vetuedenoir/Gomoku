#include "doctest.h"
#include "ai/MoveGenerator.hpp"
#include "game/board/GameBoard.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/helpers_19.hpp"

TEST_CASE("TEST_CASE [19x19] MoveGenerator: center stone radius=1 produces eight legal moves")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: center stone radius=1 produces eight legal moves"
			  << RESET << std::endl;
	GameBoard b = empty_board();
	place(b, 9, 9, CellStatus::Black);

	MoveGenerator19 gen(1);
	auto            moves = legalMovesList(gen, to_bb(b), Color::White);

	t_BWBoard19 board          = to_bb(b);
	bitboard19  legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	/*Bitboard 19*/
	CHECK(popcount_bb(legalMovesMask) == 8);
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	/*Moves*/
	CHECK(moves.size() == 8);
	CHECK_FALSE(contains_move(moves, 9, 9));

	for (int dy = -1; dy <= 1; dy++)
		for (int dx = -1; dx <= 1; dx++)
			if (dx != 0 || dy != 0)
				CHECK(contains_move(moves, 9 + dx, 9 + dy));
}

TEST_CASE("TEST_CASE [19x19] MoveGenerator: center stone radius=2 produces twenty-four legal moves")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: center stone radius=2 produces twenty-four legal moves"
			  << RESET << std::endl;
	GameBoard b = empty_board();
	place(b, 9, 9, CellStatus::Black);

	MoveGenerator19 gen(2);
	auto            moves = legalMovesList(gen, to_bb(b), Color::Black);

	t_BWBoard19 board          = to_bb(b);
	bitboard19  legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	/*Bitboard 19*/
	CHECK(popcount_bb(legalMovesMask) == 24);
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	/*Moves*/
	CHECK(moves.size() == 24);
	CHECK_FALSE(contains_move(moves, 9, 9));

	for (int dy = -1; dy <= 1; dy++)
		for (int dx = -1; dx <= 1; dx++)
			if (dx != 0 || dy != 0)
				CHECK(contains_move(moves, 9 + dx, 9 + dy));
}

TEST_CASE("TEST_CASE [19x19] MoveGenerator: corner stone radius=1 clips to board")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: corner stone radius=1 clips to board" << RESET
			  << std::endl;
	GameBoard b = empty_board();
	place(b, 0, 0, CellStatus::Black);

	MoveGenerator19 gen(1);
	auto            moves = legalMovesList(gen, to_bb(b), Color::White);

	t_BWBoard19 board          = to_bb(b);
	bitboard19  legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::White, legalMovesMask);

	/*Bitboard 19*/
	CHECK(popcount_bb(legalMovesMask) == 3);
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	/*Moves*/
	CHECK(moves.size() == 3);
	CHECK_FALSE(contains_move(moves, 0, 0));
	CHECK(contains_move(moves, 1, 0));
	CHECK(contains_move(moves, 0, 1));
	CHECK(contains_move(moves, 1, 1));
}

TEST_CASE("TEST_CASE [19x19] MoveGenerator: corner stone radius=2 clips to board")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: corner stone radius=2 clips to board" << RESET
			  << std::endl;
	GameBoard b = empty_board();
	place(b, 0, 0, CellStatus::Black);

	MoveGenerator19 gen(2);
	auto            moves = legalMovesList(gen, to_bb(b), Color::Black);

	t_BWBoard19 board          = to_bb(b);
	bitboard19  legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	/*Bitboard 19*/
	CHECK(popcount_bb(legalMovesMask) == 8);
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	/*Moves*/
	CHECK(moves.size() == 8);
	CHECK_FALSE(contains_move(moves, 0, 0));
	CHECK(contains_move(moves, 2, 0));
	CHECK(contains_move(moves, 0, 2));
	CHECK(contains_move(moves, 2, 2));
	CHECK(contains_move(moves, 1, 2));
	CHECK(contains_move(moves, 2, 1));
}

TEST_CASE("TEST_CASE [19x19] MoveGenerator: adjacent occupied cells stay excluded")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: adjacent occupied cells stay excluded" << RESET
			  << std::endl;
	GameBoard b = empty_board();
	place(b, 9, 9, CellStatus::Black);
	place(b, 10, 9, CellStatus::White);

	MoveGenerator19 gen(1);
	auto            moves = legalMovesList(gen, to_bb(b), Color::Black);

	/*Bitboard 19*/
	t_BWBoard19 board          = to_bb(b);
	bitboard19  legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);
	/*Moves*/
	CHECK(moves.size() == 10);
	CHECK_FALSE(contains_move(moves, 9, 9));
	CHECK_FALSE(contains_move(moves, 10, 9));
	CHECK(contains_move(moves, 8, 9));
	CHECK(contains_move(moves, 11, 9));
}

TEST_CASE("TEST_CASE [19x19] MoveGenerator: isLegalMove rejects out-of-board and occupied cells")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: isLegalMove rejects out-of-board and occupied cells"
			  << RESET << std::endl;
	GameBoard b = empty_board();
	place(b, 5, 5, CellStatus::Black);

	MoveGenerator19 gen(1);
	t_BWBoard19     board = to_bb(b);

	CHECK(gen.isLegalMove(board, 6, 5, Color::White));
	CHECK_FALSE(gen.isLegalMove(board, 5, 5, Color::White));
	CHECK_FALSE(gen.isLegalMove(board, -1, 5, Color::White));
	CHECK_FALSE(gen.isLegalMove(board, 19, 5, Color::White));
}

// ── Double-three rule ─────────────────────────────────────────────────────────
//
// Setup: Black stones at (3,9)-(4,9) (horizontal arm) and (5,7)-(5,8) (vertical arm).
// Playing at (5,9) simultaneously closes two free-threes → double-three → illegal.
// The move must be absent from generateMoves; a neighbouring legal move must be present.

TEST_CASE("TEST_CASE [19x19] MoveGenerator: double-three move is absent from result")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: double-three move is absent from result" << RESET
			  << std::endl;
	GameBoard b = empty_board();
	place(b, 3, 9, CellStatus::Black);
	place(b, 4, 9, CellStatus::Black);
	place(b, 5, 7, CellStatus::Black);
	place(b, 5, 8, CellStatus::Black);

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[double-three — (5,9) must be absent] Horizontal: (3,9)(4,9), Vertical: (5,7)(5,8)\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK_FALSE(contains_move(moves, 5, 9)); // double-three → filtered out
	CHECK(contains_move(moves, 3, 7));       // unambiguously legal neighbour → present
}

// Setup: only one free-three arm (horizontal): (3,9)-(4,9).
// Playing at (5,9) creates a single free-three → legal → must appear in generateMoves.

TEST_CASE("TEST_CASE [19x19] MoveGenerator: single free-three move is present in result")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: single free-three move is present in result" << RESET
			  << std::endl;
	GameBoard b = empty_board();
	place(b, 3, 9, CellStatus::Black);
	place(b, 4, 9, CellStatus::Black);

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[single free-three — (5,9) must be present] Horizontal arm: (3,9)(4,9)\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK(contains_move(moves, 5, 9)); // single free-three → legal → present
	CHECK(contains_move(moves, 2, 9)); // another legal candidate in zone → present
}

TEST_CASE("TEST_CASE [19x19] MoveGenerator: double-three is legal when move also captures")
{
	std::cout << TITLE_LINE << "TEST_CASE [19x19] MoveGenerator: double-three is legal when move also captures" << RESET
			  << std::endl;
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

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[double-three + capture legal] Board before move — candidate: (5,9) Black\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK(contains_move(moves, 5, 9));
	CHECK_FALSE(contains_move(moves, 6, 9));
	CHECK_FALSE(contains_move(moves, 7, 9));
	CHECK_FALSE(contains_move(moves, 8, 9));
}

// ── Edge cases ────────────────────────────────────────────────────────────────

// Case 1: vertical arm starts at row 0 (top edge).
// The three (5,0)-(5,1)-(5,2) has its top neighbour off-board
// which passes the >256 filter and is still counted as a free three.
// Combined with a horizontal arm (3,2)-(4,2)-(5,2): double-three → illegal.
TEST_CASE("TEST_CASE [19x19] MoveGenerator: double-three with vertical arm touching top edge is legal because it has "
          "only a horizontal free-three")
{
	std::cout << TITLE_LINE
			  << "TEST_CASE [19x19] MoveGenerator: double-three with vertical arm touching top edge is legal because "
	             "it has only a horizontal free-three"
			  << RESET << std::endl;
	GameBoard b = empty_board();
	place(b, 5, 0, CellStatus::Black);
	place(b, 5, 1, CellStatus::Black);
	// Horizontal arm
	place(b, 3, 2, CellStatus::Black);
	place(b, 4, 2, CellStatus::Black);

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);

	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[top-edge double-three illegal] Vertical arm (5,0)-(5,1)-(5,2), horizontal (3,2)-(4,2)-(5,2)\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK(contains_move(moves, 5, 2)); // only horizontal free-three → legal
	CHECK(contains_move(moves, 3, 0)); // legal neighbour in zone → present
}

// Case 2: true double-three with both arms open, no capture possible.
// Vertical arm: (5,2)-(5,3), ends (5,1) and (5,5) empty → free three.
// Horizontal arm: (3,4)-(4,4), ends (2,4) and (6,4) empty → free three.
// Candidate (5,4) closes both → double-three → must be absent from generateMoves.

TEST_CASE("[19x19] MoveGenerator: generateMoves: double-three without capture is absent from result")
{
	std::cout << TITLE_LINE
			  << "[19x19] MoveGenerator: generateMoves: double-three without capture is absent from result" << RESET
			  << std::endl;
	GameBoard b = empty_board();
	place(b, 5, 2, CellStatus::Black);
	place(b, 5, 3, CellStatus::Black);
	place(b, 3, 4, CellStatus::Black);
	place(b, 4, 4, CellStatus::Black);

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[double-three, no capture — (5,4) must be absent]\n";
	std::cout << "  Vertical: (5,2)(5,3), Horizontal: (3,4)(4,4), candidate: (5,4)\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK_FALSE(contains_move(moves, 5, 4)); // double-three, no capture → illegal → absent
	CHECK(contains_move(moves, 3, 2));       // unambiguously legal neighbour → present
}

// Case 3: same double-three setup but a diagonal capture is possible.
// Capture pattern: (5,4)B - (6,5)W - (7,6)W - (8,7)B (diagonal direction).
// The capture is on the diagonal so it does NOT block either arm's open end.
// Capture exempts the double-three rule → (5,4) must be present in generateMoves.

TEST_CASE("[19x19] MoveGenerator: generateMoves: double-three with capture is present in result")
{
	std::cout << TITLE_LINE << "[19x19] MoveGenerator: generateMoves: double-three with capture is present in result"
			  << RESET << std::endl;
	GameBoard b = empty_board();
	// Same double-three arms as Case 2a
	place(b, 5, 2, CellStatus::Black);
	place(b, 5, 3, CellStatus::Black);
	place(b, 3, 4, CellStatus::Black);
	place(b, 4, 4, CellStatus::Black);
	// Diagonal capture: playing B@(5,4) flanks (6,5)W and (7,6)W with B@(8,7)
	place(b, 6, 5, CellStatus::White);
	place(b, 7, 6, CellStatus::White);
	place(b, 8, 7, CellStatus::Black);

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[double-three + diagonal capture — (5,4) must be present]\n";
	std::cout << "  Capture: B@(5,4) W@(6,5) W@(7,6) B@(8,7) — diagonal, does not block arms\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK(contains_move(moves, 5, 4)); // capture exempts double-three → legal → present
}

// Case 4: double-three with an insufficient diagonal capture attempt.
// Horizontal arm: (14,9)-(15,9), vertical arm: (16,7)-(16,8), candidate: (16,9).
// Both arms have fully open ends — genuine double-three.
// A lone W@(18,9) on the diagonal has no Black anchor behind it:
// detect_captures returns false → double-three stands → (16,9) must be absent.

TEST_CASE("[19x19] MoveGenerator: double-three with insufficient capture (lone white, no anchor) is absent")
{
	std::cout << TITLE_LINE
			  << "[19x19] MoveGenerator: double-three with insufficient capture (lone white, no anchor) is absent"
			  << RESET << std::endl;
	GameBoard b = empty_board();
	// Horizontal arm — right end (17,9) kept empty so the arm stays a free-three
	place(b, 14, 9, CellStatus::Black);
	place(b, 15, 9, CellStatus::Black);
	// Vertical arm — both ends open
	place(b, 16, 7, CellStatus::Black);
	place(b, 16, 8, CellStatus::Black);
	// Lone white on the diagonal: no second white, no Black anchor → not a valid capture
	place(b, 18, 9, CellStatus::White);

	MoveGenerator19 gen(2);
	t_BWBoard19     board = to_bb(b);

	bitboard19 legalMovesMask = {};
	gen.getMaskOfLegalMoves(board, Color::Black, legalMovesMask);
	auto moves = legalMovesList(gen, board, Color::Black);

	std::cout << "\n[double-three, lone W@(18,9) — no anchor → no capture] Candidate: (16,9) Black\n";
	std::cout << "  Capture needs B-W-W-B; only one W present → detect_captures = false\n";
	print_bb_overlay<BoardTraits<19> >(board, legalMovesMask);

	CHECK_FALSE(contains_move(moves, 16, 9)); // double-three, no valid capture → absent
	CHECK(contains_move(moves, 14, 7));       // unambiguously legal neighbour → present
}
