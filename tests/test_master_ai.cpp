#include "doctest.h"
#include "ai/MoveGenerator.hpp"
#include "ai/SearchPosition.hpp"
#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "test_helpers.hpp"
#include <iostream>

// ── File-local helpers ────────────────────────────────────────────────────────

static void place(GameBoard& b, int col, int row, CellStatus color)
{
	b.placeStoneOfColor(col, row, color);
}

static void print_moves(const std::vector<t_cell>& moves)
{
	std::cout << "  moves (" << moves.size() << "):\n";
	for (const auto& m : moves)
		std::cout << "    (" << m.x << ", " << m.y << ")\n";
}

// print_bb_19 takes a non-const ref; copy the board first.
static void print_board(const SearchPosition19& pos)
{
	auto board = pos.board();
	print_bb_19(board);	
}

// ── Test cases ────────────────────────────────────────────────────────────────

// ── 1. Empty board ────────────────────────────────────────────────────────────
//
// With no stones on the board the active zone is completely empty.
// generateMoves must return an empty vector — the same guard that causes
// minimax (line 43 of MasterAI.inl) to fall through to evaluatePosition.

TEST_CASE("MasterAI::generateMoves: empty board produces no candidates")
{
	GameBoard b(19, Color::Black);
	SearchPosition19 pos = SearchPosition19::fromBoard(b);

	MoveGenerator19 gen(2);
	auto moves = gen.generateMoves(pos.board(), pos.sideToMove());

	std::cout << "\n[empty board] sideToMove=Black\n";
	print_board(pos);
	print_moves(moves);
	std::cout << "  Expected: 0 moves (active zone has no seeds)\n";

	CHECK(moves.empty());
}

// ── 2. Single center stone — radius=2 produces 24 legal candidates ────────────
//
// A single Black stone at (9,9) seeds a 5×5 active zone (radius=2).
// All 24 surrounding cells are empty and legal → 24 moves expected.
// The occupied cell (9,9) must never appear in the result.

TEST_CASE("MasterAI::generateMoves: single center stone yields 24 candidates at radius=2")
{
	GameBoard b(19, Color::Black);
	place(b, 9, 9, CellStatus::Black);
	SearchPosition19 pos = SearchPosition19::fromBoard(b);

	MoveGenerator19 gen(2);
	auto moves = gen.generateMoves(pos.board(), pos.sideToMove());

	std::cout << "\n[single center stone B@(9,9)] sideToMove=Black, radius=2\n";
	print_board(pos);
	print_moves(moves);
	std::cout << "  Expected: 24 moves (5x5 zone minus the occupied center)\n";

	CHECK(moves.size() == 24);
	CHECK_FALSE(contains_move(moves, 9, 9));   // occupied — must be excluded
	CHECK(contains_move(moves, 7, 7));          // top-left corner of zone
	CHECK(contains_move(moves, 11, 11));        // bottom-right corner of zone
	CHECK(contains_move(moves, 9, 7));          // top edge of zone
	CHECK(contains_move(moves, 7, 9));          // left edge of zone
}

// ── 3. sideToMove() propagates to the legality filter ────────────────────────
//
// Setup: Black stones at (3,9),(4,9) and (5,7),(5,8).
// Playing at (5,9) simultaneously creates two free-threes for Black → double-three
// → illegal for Black.  White playing at (5,9) forms no double-three at all
// → legal for White.
//
// This directly verifies that position.sideToMove() is forwarded correctly
// from minimax line 42 all the way into StandardRules::isLegal.

// TEST_CASE("MasterAI::generateMoves: sideToMove propagates to double-three legality filter")
// {
// 	SUBCASE("Black to move — double-three at (5,9) is filtered out")
// 	{
// 		GameBoard b(19, Color::Black);
// 		place(b, 3, 9, CellStatus::Black);
// 		place(b, 4, 9, CellStatus::Black);
// 		place(b, 5, 7, CellStatus::Black);
// 		place(b, 5, 8, CellStatus::Black);

// 		SearchPosition19 pos = SearchPosition19::fromBoard(b);
// 		MoveGenerator19 gen(2);
// 		auto moves = gen.generateMoves(pos.board(), pos.sideToMove());

// 		std::cout << "\n[double-three — Black to move] candidate (5,9)\n";
// 		std::cout << "  Horizontal arm: (3,9)B (4,9)B -> (5,9)\n";
// 		std::cout << "  Vertical   arm: (5,7)B (5,8)B -> (5,9)\n";
// 		print_board(pos);
// 		std::cout << "  sideToMove = Black\n";
// 		print_moves(moves);
// 		std::cout << "  Expected: (5,9) absent — double-three is illegal for Black\n";

// 		CHECK(pos.sideToMove() == Color::Black);
// 		CHECK_FALSE(contains_move(moves, 5, 9));
// 	}

// 	SUBCASE("White to move — same Black stones, (5,9) is legal for White")
// 	{
// 		GameBoard b(19, Seat::Second);
// 		place(b, 3, 9, CellStatus::Black);
// 		place(b, 4, 9, CellStatus::Black);
// 		place(b, 5, 7, CellStatus::Black);
// 		place(b, 5, 8, CellStatus::Black);

// 		SearchPosition19 pos = SearchPosition19::fromBoard(b);
// 		MoveGenerator19 gen(2);
// 		auto moves = gen.generateMoves(pos.board(), pos.sideToMove());

// 		std::cout << "\n[double-three — White to move] candidate (5,9)\n";
// 		std::cout << "  Same board, but White is to play — White forms no double-three here\n";
// 		print_board(pos);
// 		std::cout << "  sideToMove = White\n";
// 		print_moves(moves);
// 		std::cout << "  Expected: (5,9) present — legal for White\n";

// 		CHECK(pos.sideToMove() == Color::White);
// 		CHECK(contains_move(moves, 5, 9));
// 	}
// }

// ── 4. makeMove flips sideToMove — moves reflect the new player ──────────────
//
// Simulates what minimax does when recursing: after makeMove the side flips,
// and the next generateMoves call must use the updated sideToMove.
// Starting from an empty board Black plays (9,9); the position then belongs
// to White, and generateMoves should enumerate White's legal replies.

// TEST_CASE("MasterAI::generateMoves: after makeMove sideToMove flips and moves update")
// {
// 	GameBoard b(19, Color::Black);
// 	SearchPosition19 pos = SearchPosition19::fromBoard(b);

// 	std::cout << "\n[makeMove flip]\n";
// 	std::cout << "  before makeMove: sideToMove="
// 	          << (pos.sideToMove() == Color::Black ? "Black" : "White") << "\n";

// 	pos.makeMove(9, 9, CellStatus::Black);

// 	std::cout << "  after makeMove(9,9,Black): sideToMove="
// 	          << (pos.sideToMove() == Color::Black ? "Black" : "White") << "\n";

// 	MoveGenerator19 gen(2);
// 	auto moves = gen.generateMoves(pos.board(), pos.sideToMove());

// 	print_board(pos);
// 	print_moves(moves);
// 	std::cout << "  Expected: sideToMove=White, 24 moves around (9,9)\n";

// 	CHECK(pos.sideToMove() == Color::White);
// 	CHECK(moves.size() == 24);
// 	CHECK_FALSE(contains_move(moves, 9, 9));    // occupied by Black
// 	CHECK(contains_move(moves, 10, 9));
// 	CHECK(contains_move(moves, 9, 10));
// 	CHECK(contains_move(moves, 8, 8));
// }

// ── 5. Active zone unions multiple seeds — evaluatePosition empty-guard path ──
//
// The moves.empty() guard in minimax (line 43 of MasterAI.inl) is triggered
// only when there are no seeds on the board (i.e. the position is empty), as
// demonstrated by test 1.  This test verifies the complementary invariant:
// once two stones exist at opposite corners the zone correctly unions both
// seeds and produces candidates around each, giving minimax a non-empty move
// list to recurse into.
//
// Board: Black@(0,0) and Black@(18,18).  With radius=2 the zone around (0,0)
// contributes the 3×3 corner clip (8 cells) and the zone around (18,18)
// contributes another 3×3 corner clip (8 cells) — 16 candidates total, both
// stones excluded.

// TEST_CASE("MasterAI::generateMoves: two corner stones produce unioned zone — non-empty for minimax recursion")
// {
// 	GameBoard b(19, Color::Black);
// 	place(b, 0,  0,  CellStatus::Black);
// 	place(b, 18, 18, CellStatus::Black);

// 	SearchPosition19 pos = SearchPosition19::fromBoard(b);
// 	MoveGenerator19 gen(2);
// 	auto moves = gen.generateMoves(pos.board(), pos.sideToMove());

// 	std::cout << "\n[two corner stones] B@(0,0) and B@(18,18), radius=2\n";
// 	print_board(pos);
// 	print_moves(moves);
// 	std::cout << "  Expected: 16 moves (3x3 clip at each corner minus 2 occupied)\n";
// 	std::cout << "  Both seeds unioned → minimax can recurse (moves not empty)\n";

// 	CHECK(moves.size() == 16);

// 	// Top-left corner zone (clips to board edge)
// 	CHECK(contains_move(moves, 1, 0));
// 	CHECK(contains_move(moves, 0, 1));
// 	CHECK(contains_move(moves, 1, 1));
// 	CHECK_FALSE(contains_move(moves, 0, 0));   // occupied by Black

// 	// Bottom-right corner zone (clips to board edge)
// 	CHECK(contains_move(moves, 17, 18));
// 	CHECK(contains_move(moves, 18, 17));
// 	CHECK(contains_move(moves, 17, 17));
// 	CHECK_FALSE(contains_move(moves, 18, 18)); // occupied by Black

// 	// The two zones must not bleed into each other (board is large enough)
// 	CHECK_FALSE(contains_move(moves, 9, 9));
// }
