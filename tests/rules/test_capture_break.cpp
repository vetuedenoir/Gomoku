#include "doctest.h"

#include "bitboard/BitboardTool.hpp"
#include "bitboard/bitboard.hpp"
#include "game/turn/WinDetector.hpp"

#include <vector>

// =============================================================================
// Endgame-capture rule primitives.
//
//   canBreakFive     — can the defender take a pair holding a stone of the five?
//   hasAnyCapture    — can a player take any pair at all, anywhere?
//   isFiveRefutable  — the only escape: a capture that breaks the line. A
//                      capture elsewhere never refutes a five, whatever the
//                      defender's capture count.
//   hasDrawingBreak  — the breaking capture also brings the defender to ten
//                      captured stones: both wins land on the same move, draw.
//
// A contiguous five can never be broken along its own axis (both flanks of an
// inner pair are line stones, and an end pair always has one), so every test
// below breaks it through a pair perpendicular or diagonal to the line.
// =============================================================================

using T19  = BoardTraits<19>;
using BB19 = typename T19::Bitboard;

namespace
{

	struct Cell
	{
		int x, y;
	};

	void put(BB19& bb, const std::vector<Cell>& cells)
	{
		for (const Cell& c : cells)
			set_bb_generic<T19>(bb, c.x, c.y);
	}

	// Black five on row 5, x in [3..7]. Returns the board; `mask` gets its cells.
	t_BWBoard19 board_with_black_five(BB19& mask)
	{
		t_BWBoard19 board = {};
		mask              = BB19{};
		for (int x = 3; x <= 7; x++)
		{
			set_bb_generic<T19>(board.black, x, 5);
			set_bb_generic<T19>(mask, x, 5);
		}
		return board;
	}

} // namespace

// ─── Nothing to capture ──────────────────────────────────────────────────────

TEST_CASE("canBreakFive: a lone five with no white stone is unbreakable")
{
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
	CHECK(hasAnyCapture<T19>(board, Color::White) == false);
}

TEST_CASE("canBreakFive: a contiguous five cannot be broken along its own axis")
{
	// White flanks the line on both ends; no perpendicular pair exists, and the
	// in-line pairs all have a black stone on one flank.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.white, { { 2, 5 }, { 8, 5 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
}

// ─── Breaking through a perpendicular pair ───────────────────────────────────

TEST_CASE("canBreakFive: vertical pair anchored below, landing above")
{
	// (5,4) empty · (5,5) black (in the five) · (5,6) black · (5,7) white
	// White plays (5,4) and takes (5,5)+(5,6): the line loses (5,5).
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 } });
	put(board.white, { { 5, 7 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == true);
}

TEST_CASE("canBreakFive: vertical pair anchored above, landing below")
{
	// (5,4) white · (5,5) black · (5,6) black · (5,7) empty — mirror of above.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 } });
	put(board.white, { { 5, 4 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == true);
}

TEST_CASE("canBreakFive: the line stone may be the far member of the pair")
{
	// (5,3) white · (5,4) black · (5,5) black (in the five) · (5,6) empty.
	// White plays (5,6) and takes (5,5)+(5,4): the scan must consider the pair
	// both ways round, not only "line stone first".
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 4 } });
	put(board.white, { { 5, 3 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == true);
}

TEST_CASE("canBreakFive: diagonal pair breaks the line")
{
	// (4,4) empty · (5,5) black (in the five) · (6,6) black · (7,7) white
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 6, 6 } });
	put(board.white, { { 7, 7 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == true);
}

TEST_CASE("canBreakFive: every stone of the line is checked, not just the middle")
{
	// The breakable pair hangs off the last stone (7,5).
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 7, 6 } });
	put(board.white, { { 7, 7 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == true);
}

// ─── Patterns that look breakable but are not ────────────────────────────────

TEST_CASE("canBreakFive: no anchor stone means no capture")
{
	// (5,4) empty · (5,5) black · (5,6) black · (5,7) empty — nothing to take.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
}

TEST_CASE("canBreakFive: an occupied landing cell blocks the capture")
{
	// (5,3) black · (5,4) black · (5,5) black · (5,6) black · (5,7) white:
	// the pair is real and anchored, but white has nowhere to drop the stone.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 }, { 5, 4 }, { 5, 3 } });
	put(board.white, { { 5, 7 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
}

TEST_CASE("canBreakFive: three black stones in a row are not a capturable pair")
{
	// (5,4) empty · (5,5)(5,6)(5,7) black · (5,8) white — a triplet is immune.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 }, { 5, 7 } });
	put(board.white, { { 5, 8 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
}

TEST_CASE("canBreakFive: a capture available off the line does not break it")
{
	// A takeable black pair far from row 5: white can capture, but the five
	// survives, so the alignment still wins on its own.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 12, 12 }, { 13, 13 } });
	put(board.white, { { 14, 14 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
	CHECK(hasAnyCapture<T19>(board, Color::White) == true);
}

TEST_CASE("canBreakFive: the five's own colour cannot break it")
{
	// Black to capture black is meaningless; the predicate is asked from the
	// defender's side only, so a black-owned pair with a black anchor is inert.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 }, { 5, 7 } });

	CHECK(canBreakFive<T19>(board, Color::Black, five) == false);
}

// ─── hasAnyCapture ───────────────────────────────────────────────────────────

TEST_CASE("hasAnyCapture: empty board has no capture")
{
	t_BWBoard19 board = {};

	CHECK(hasAnyCapture<T19>(board, Color::White) == false);
	CHECK(hasAnyCapture<T19>(board, Color::Black) == false);
}

TEST_CASE("hasAnyCapture: is asymmetric between the two colours")
{
	// (9,8) empty · (9,9) black · (9,10) black · (9,11) white
	t_BWBoard19 board = {};
	put(board.black, { { 9, 9 }, { 9, 10 } });
	put(board.white, { { 9, 11 } });

	CHECK(hasAnyCapture<T19>(board, Color::White) == true);
	CHECK(hasAnyCapture<T19>(board, Color::Black) == false);
}

TEST_CASE("hasAnyCapture: a capture at the board edge with no landing cell")
{
	// (0,0)(0,1) black · (0,2) white — the landing cell would be (0,-1).
	t_BWBoard19 board = {};
	put(board.black, { { 0, 0 }, { 0, 1 } });
	put(board.white, { { 0, 2 } });

	CHECK(hasAnyCapture<T19>(board, Color::White) == false);
}

TEST_CASE("hasAnyCapture: agrees with detect_captures on the landing cell")
{
	// Whatever hasAnyCapture claims must be reproducible by actually playing:
	// sweep every empty cell and look for a real capture.
	t_BWBoard19 board = {};
	put(board.black, { { 9, 9 }, { 9, 10 } });
	put(board.white, { { 9, 11 } });

	bool foundByPlaying = false;
	for (int y = 0; y < 19 && !foundByPlaying; ++y)
	{
		for (int x = 0; x < 19; ++x)
		{
			if (!isEmptyCell<T19>(board, x, y))
				continue;

			BB19 captured = {};
			if (detect_captures<T19>(board, x, y, Color::White, captured))
			{
				foundByPlaying = true;
				break;
			}
		}
	}

	CHECK(hasAnyCapture<T19>(board, Color::White) == foundByPlaying);
}

// ─── isFiveRefutable ─────────────────────────────────────────────────────────

TEST_CASE("isFiveRefutable: an unbreakable five with no capture around wins")
{
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);

	CHECK(isFiveRefutable<T19>(board, Color::Black, five) == false);
}

TEST_CASE("isFiveRefutable: a breakable five is refutable")
{
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 } });
	put(board.white, { { 5, 7 } });

	CHECK(isFiveRefutable<T19>(board, Color::Black, five) == true);
}

TEST_CASE("isFiveRefutable: a capture far from the line never refutes the five")
{
	// The only capture available is far from the line. Under the endgame rule
	// it is irrelevant, even with four pairs already lost: the alignment
	// resolves first.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 12, 12 }, { 13, 13 } });
	put(board.white, { { 14, 14 } });

	CHECK(hasAnyCapture<T19>(board, Color::White) == true);
	CHECK(isFiveRefutable<T19>(board, Color::Black, five) == false);
}

// ─── hasDrawingBreak ─────────────────────────────────────────────────────────

TEST_CASE("hasDrawingBreak: a break that does not reach ten is not a draw")
{
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five);
	put(board.black, { { 5, 6 } });
	put(board.white, { { 5, 7 } }); // White plays (5,4), takes 2 stones

	BB19 masks[1] = { five };

	CHECK(canBreakFive<T19>(board, Color::Black, five) == true);
	CHECK(hasDrawingBreak<T19>(board, Color::Black, masks, 1, 6) == false);
	CHECK(hasDrawingBreak<T19>(board, Color::Black, masks, 1, 8) == true);
}

TEST_CASE("hasDrawingBreak: one move taking three pairs reaches ten from four")
{
	// White lands on (5,4). Three black pairs radiate from it — vertical and
	// both diagonals — each closed by a white stone: 6 stones in one move.
	BB19        five;
	t_BWBoard19 board = board_with_black_five(five); // black row 5, x in [3..7]

	// vertical pair (5,5)+(5,6) is already half the five; close it at (5,7)
	put(board.black, { { 5, 6 } });
	put(board.white, { { 5, 7 } });
	// diagonal pair (4,5)+(3,6), closed at (2,7)
	put(board.black, { { 3, 6 } });
	put(board.white, { { 2, 7 } });
	// anti-diagonal pair (6,5)+(7,6), closed at (8,7)
	put(board.black, { { 7, 6 } });
	put(board.white, { { 8, 7 } });

	BB19 captured{};
	detect_captures<T19>(board, 5, 4, Color::White, captured);
	REQUIRE(popcount_bb_generic<T19>(captured) == 6);

	BB19 masks[1] = { five };

	CHECK(hasDrawingBreak<T19>(board, Color::Black, masks, 1, 4) == true);
	CHECK(hasDrawingBreak<T19>(board, Color::Black, masks, 1, 2) == false);
}

// ─── White-side symmetry ─────────────────────────────────────────────────────

TEST_CASE("canBreakFive: the rule works identically for a white alignment")
{
	t_BWBoard19 board = {};
	BB19        five  = {};
	for (int x = 3; x <= 7; x++)
	{
		set_bb_generic<T19>(board.white, x, 5);
		set_bb_generic<T19>(five, x, 5);
	}

	CHECK(canBreakFive<T19>(board, Color::White, five) == false);

	put(board.white, { { 5, 6 } });
	put(board.black, { { 5, 7 } });

	CHECK(canBreakFive<T19>(board, Color::White, five) == true);
	CHECK(isFiveRefutable<T19>(board, Color::White, five) == true);
}

// ─── Wiring with find_five_masks ─────────────────────────────────────────────

TEST_CASE("find_five_masks + canBreakFive: only the unbreakable alignment wins")
{
	// Six black stones on row 5 → two candidate fives through (5,5).
	// A white anchor breaks the pair (3,5)+(3,6), which belongs to the first
	// alignment only: the second one survives, so the position is still a win.
	t_BWBoard19 board = {};
	for (int x = 3; x <= 8; x++)
		set_bb_generic<T19>(board.black, x, 5);
	put(board.black, { { 3, 6 } });
	put(board.white, { { 3, 7 } }); // landing cell (3,4) is empty

	BB19      masks[BitboardTool19::MAX_FIVE_MASKS];
	const int n = BitboardTool19::instance().find_five_masks(board.black, 5, 5, masks, BitboardTool19::MAX_FIVE_MASKS);

	REQUIRE(n == 2);

	int unbreakable = 0;
	for (int i = 0; i < n; ++i)
	{
		if (!canBreakFive<T19>(board, Color::Black, masks[i]))
			++unbreakable;
	}

	// 3..7 contains (3,5) and is breakable; 4..8 does not and is not.
	CHECK(unbreakable == 1);
}
