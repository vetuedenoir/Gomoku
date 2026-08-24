#include "doctest.h"

#include "game/turn/TurnController.hpp"

#include <optional>
#include <vector>

// =============================================================================
// TurnController + PendingWin — the endgame-capture rule end to end.
//
// A five wins on the spot unless the opponent can capture a pair that contains
// one of its stones. That is the only deferral: a capture elsewhere does not
// count, even if it would complete the defender's five pairs — the alignment
// resolves first. If the five is refutable, it is deferred by exactly one ply.
// =============================================================================

using T19 = BoardTraits<19>;

namespace
{

	struct Cell
	{
		int x, y;
	};

	// Mirrors GameController's bookkeeping around TurnController::play so the tests
	// exercise the real hand-off (capture counters in, pending state through).
	struct Table
	{
		t_BWBoard19               bb{};
		int                       capturesBlack = 0;
		int                       capturesWhite = 0;
		std::optional<PendingWin> pending;
		TurnController<T19>       turn;

		void place(Color c, const std::vector<Cell>& cells)
		{
			for (const Cell& cell : cells)
				set_bb_generic<T19>(bitboardForColor<T19>(bb, c), cell.x, cell.y);
		}

		TurnOutcome<T19> play(Color c, int x, int y)
		{
			const Move             m{ x, y, colorToCell(c) };
			const TurnOutcome<T19> out = turn.play(bb, m, capturesBlack, capturesWhite, pending);

			if (c == Color::Black)
				capturesBlack += out.capturesAdded;
			else
				capturesWhite += out.capturesAdded;
			pending = out.pendingWin;

			return out;
		}
	};

	// Black has 3,4,5,6 on row 5 and is one stone away from a five at (7,5).
	void four_black_on_row5(Table& t)
	{
		t.place(Color::Black, { { 3, 5 }, { 4, 5 }, { 5, 5 }, { 6, 5 } });
	}

	// The pair (5,5)+(5,6) is takeable by White at (5,4): breaks a five on row 5.
	void breakable_pair_on_row5(Table& t)
	{
		t.place(Color::Black, { { 5, 6 } });
		t.place(Color::White, { { 5, 7 } });
	}

} // namespace

// ─── Immediate win ───────────────────────────────────────────────────────────

TEST_CASE("[PENDING_WIN] an unbreakable five wins on the spot")
{
	Table t;
	four_black_on_row5(t);

	const TurnOutcome<T19> out = t.play(Color::Black, 7, 5);

	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::Black);
	CHECK(out.pendingWin.has_value() == false);
}

TEST_CASE("[PENDING_WIN] the tenth capture still wins immediately")
{
	Table t;
	t.place(Color::Black, { { 9, 9 }, { 9, 10 } });
	t.place(Color::White, { { 9, 11 } });
	t.capturesWhite = 8;

	const TurnOutcome<T19> out = t.play(Color::White, 9, 8);

	CHECK(out.capturesAdded == 2);
	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::White);
	CHECK(out.pendingWin.has_value() == false);
}

TEST_CASE("[PENDING_WIN] a quiet move ends with no winner and no pending five")
{
	Table t;
	four_black_on_row5(t);

	const TurnOutcome<T19> out = t.play(Color::White, 15, 15);

	CHECK(out.result == MoveResult::Ok);
	CHECK(out.winnerByColor.has_value() == false);
	CHECK(out.pendingWin.has_value() == false);
}

// ─── Deferred win ────────────────────────────────────────────────────────────

TEST_CASE("[PENDING_WIN] a breakable five does not end the game")
{
	Table t;
	four_black_on_row5(t);
	breakable_pair_on_row5(t);

	const TurnOutcome<T19> out = t.play(Color::Black, 7, 5);

	CHECK(out.result == MoveResult::Ok);
	CHECK(out.winnerByColor.has_value() == false);
	REQUIRE(out.pendingWin.has_value());
	CHECK(out.pendingWin->owner == Color::Black);
	CHECK(out.pendingWin->col == 7);
	CHECK(out.pendingWin->row == 5);
}

TEST_CASE("[PENDING_WIN] an unbroken five wins one ply later")
{
	Table t;
	four_black_on_row5(t);
	breakable_pair_on_row5(t);

	REQUIRE(t.play(Color::Black, 7, 5).result == MoveResult::Ok);

	// White ignores the threat.
	const TurnOutcome<T19> out = t.play(Color::White, 15, 15);

	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::Black);
	CHECK(out.pendingWin.has_value() == false);
}

TEST_CASE("[PENDING_WIN] breaking the line clears the pending win and play resumes")
{
	Table t;
	four_black_on_row5(t);
	breakable_pair_on_row5(t);

	REQUIRE(t.play(Color::Black, 7, 5).result == MoveResult::Ok);

	// White plays (5,4) and takes (5,5)+(5,6): the row-5 alignment is gone.
	const TurnOutcome<T19> out = t.play(Color::White, 5, 4);

	CHECK(out.capturesAdded == 2);
	CHECK(out.result == MoveResult::Ok);
	CHECK(out.winnerByColor.has_value() == false);
	CHECK(out.pendingWin.has_value() == false);
	CHECK(get_bb_generic<T19>(t.bb.black, 5, 5) == false);
}

TEST_CASE("[PENDING_WIN] capturing the stone that completed the five also breaks it")
{
	// The pair hangs off (7,5) itself, so the anchor cell disappears — the
	// survival check must cope with its own reference stone being gone.
	Table t;
	four_black_on_row5(t);
	t.place(Color::Black, { { 7, 6 } });
	t.place(Color::White, { { 7, 7 } });

	REQUIRE(t.play(Color::Black, 7, 5).result == MoveResult::Ok);

	const TurnOutcome<T19> out = t.play(Color::White, 7, 4);

	CHECK(out.capturesAdded == 2);
	CHECK(get_bb_generic<T19>(t.bb.black, 7, 5) == false);
	CHECK(out.result == MoveResult::Ok);
	CHECK(out.pendingWin.has_value() == false);
}

// ─── A capture off the line never saves ──────────────────────────────────────

TEST_CASE("[PENDING_WIN] a far capture does not defer an unbreakable five, even at eight stones taken")
{
	// White is one pair from ten and can take (12,12)+(13,13), but that pair
	// is not on row 5, so the five is not refutable.
	Table t;
	four_black_on_row5(t);
	t.place(Color::Black, { { 12, 12 }, { 13, 13 } });
	t.place(Color::White, { { 14, 14 } });
	t.capturesWhite = 8;

	const TurnOutcome<T19> out = t.play(Color::Black, 7, 5);

	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::Black);
	CHECK(out.pendingWin.has_value() == false);
}

TEST_CASE("[PENDING_WIN] below eight stones taken, the same five wins at once")
{
	Table t;
	four_black_on_row5(t);
	t.place(Color::Black, { { 12, 12 }, { 13, 13 } });
	t.place(Color::White, { { 14, 14 } });
	t.capturesWhite = 6;

	const TurnOutcome<T19> out = t.play(Color::Black, 7, 5);

	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::Black);
}

// ─── Both sides in one move ──────────────────────────────────────────────────

TEST_CASE("[PENDING_WIN] breaking the line and aligning five in the same move wins")
{
	Table t;
	four_black_on_row5(t);
	breakable_pair_on_row5(t);
	// White is four in a row on row 4; (5,4) both breaks black's line and
	// completes white's own five.
	t.place(Color::White, { { 1, 4 }, { 2, 4 }, { 3, 4 }, { 4, 4 } });

	REQUIRE(t.play(Color::Black, 7, 5).result == MoveResult::Ok);

	const TurnOutcome<T19> out = t.play(Color::White, 5, 4);

	CHECK(out.capturesAdded == 2);
	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::White);
	CHECK(out.pendingWin.has_value() == false);
}

// ─── Several alignments through the played cell ──────────────────────────────

TEST_CASE("[PENDING_WIN] one unbreakable alignment out of two is enough to win")
{
	// Six on row 5 → the played cell (5,5) belongs to 3..7 and to 4..8.
	// Only 3..7 can be broken (through (3,5)), so the win stands.
	Table t;
	t.place(Color::Black, { { 3, 5 }, { 4, 5 }, { 6, 5 }, { 7, 5 }, { 8, 5 } });
	t.place(Color::Black, { { 3, 6 } });
	t.place(Color::White, { { 3, 7 } });

	const TurnOutcome<T19> out = t.play(Color::Black, 5, 5);

	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::Black);
}

TEST_CASE("[PENDING_WIN] two breakable alignments still defer the win")
{
	// Same six, but now 4..8 is breakable too (through (8,5)).
	Table t;
	t.place(Color::Black, { { 3, 5 }, { 4, 5 }, { 6, 5 }, { 7, 5 }, { 8, 5 } });
	t.place(Color::Black, { { 3, 6 }, { 8, 6 } });
	t.place(Color::White, { { 3, 7 }, { 8, 7 } });

	const TurnOutcome<T19> out = t.play(Color::Black, 5, 5);

	CHECK(out.result == MoveResult::Ok);
	REQUIRE(out.pendingWin.has_value());
	CHECK(out.pendingWin->col == 5);
	CHECK(out.pendingWin->row == 5);
}

TEST_CASE("[PENDING_WIN] breaking only one of two alignments loses")
{
	// White takes the pair guarding 3..7; the 4..8 alignment survives, and the
	// survival check keys on (5,5), which is a member of both.
	Table t;
	t.place(Color::Black, { { 3, 5 }, { 4, 5 }, { 6, 5 }, { 7, 5 }, { 8, 5 } });
	t.place(Color::Black, { { 3, 6 }, { 8, 6 } });
	t.place(Color::White, { { 3, 7 }, { 8, 7 } });

	REQUIRE(t.play(Color::Black, 5, 5).result == MoveResult::Ok);

	const TurnOutcome<T19> out = t.play(Color::White, 3, 4);

	CHECK(out.capturesAdded == 2);
	CHECK(get_bb_generic<T19>(t.bb.black, 3, 5) == false);
	CHECK(out.result == MoveResult::Win);
	REQUIRE(out.winnerByColor.has_value());
	CHECK(out.winnerByColor.value() == Color::Black);
}

// ─── White-side symmetry ─────────────────────────────────────────────────────

TEST_CASE("[PENDING_WIN] the deferral works the same way for White")
{
	Table t;
	t.place(Color::White, { { 3, 5 }, { 4, 5 }, { 5, 5 }, { 6, 5 } });
	t.place(Color::White, { { 5, 6 } });
	t.place(Color::Black, { { 5, 7 } });

	const TurnOutcome<T19> deferred = t.play(Color::White, 7, 5);
	CHECK(deferred.result == MoveResult::Ok);
	REQUIRE(deferred.pendingWin.has_value());
	CHECK(deferred.pendingWin->owner == Color::White);

	const TurnOutcome<T19> broken = t.play(Color::Black, 5, 4);
	CHECK(broken.capturesAdded == 2);
	CHECK(broken.result == MoveResult::Ok);
	CHECK(broken.pendingWin.has_value() == false);
}
