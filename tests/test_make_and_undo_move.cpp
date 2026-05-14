#include "doctest.h"
#include "game/GameBoard.hpp"
#include "game/Seat.hpp"
#include "optimization/SearchPosition.hpp"
#include "bitboard/bitboard.hpp"

static SearchPosition19 emptyPos()
{
    GameBoard board(19, Seat::First);
    return SearchPosition19::fromBoard(board);
}

TEST_CASE("make/undo single move restores hash")
{
    SearchPosition19 pos     = emptyPos();
    uint64_t         initial = pos.zobristHash();

    pos.makeMove(10, 10, CellStatus::Black);
    pos.undoMove(10, 10, CellStatus::Black);

    CHECK(pos.zobristHash() == initial);
}

// ── Test 2: sequence of moves fully undone restores hash ─────────────────────
//
// Validates the move stack property required by Minimax: undoing N moves in
// reverse order must restore the exact starting hash.

TEST_CASE("make/undo sequence restores hash")
{
    SearchPosition19 pos     = emptyPos();
    uint64_t         initial = pos.zobristHash();

    pos.makeMove(5, 5, CellStatus::Black);
    pos.makeMove(6, 6, CellStatus::White);
    pos.makeMove(7, 7, CellStatus::Black);

    pos.undoMove(7, 7, CellStatus::Black);
    pos.undoMove(6, 6, CellStatus::White);
    pos.undoMove(5, 5, CellStatus::Black);

    CHECK(pos.zobristHash() == initial);
}

// ── Test 3: incremental hash equals full recompute ───────────────────────────
//
// Verifies that the incremental XOR updates in makeMove stay consistent with
// a full recompute from the bitboard. Catches mapping errors between
// (col, row) ↔ bitboard index ↔ Zobrist table index.

TEST_CASE("incremental hash matches full recompute")
{
    SearchPosition19 pos = emptyPos();

    pos.makeMove(3,  3,  CellStatus::Black);
    pos.makeMove(10, 10, CellStatus::White);

    uint64_t incremental = pos.zobristHash();
    uint64_t recomputed  = SearchPosition19::hasher().compute(pos.board());

    CHECK(incremental == recomputed);
}

// ── Test 4: different stone placements produce different hashes ───────────────
//
// Two paths occupy the same squares but with opposite stone colors, making
// them genuinely distinct positions. Confirms the hash distinguishes them.
//
// Path A: Black@(5,5)  White@(6,6)
// Path B: Black@(6,6)  White@(5,5)

TEST_CASE("different stone placements produce different hashes")
{
    GameBoard boardA(19, Seat::First);
    SearchPosition19 posA = SearchPosition19::fromBoard(boardA);
    posA.makeMove(5, 5, CellStatus::Black);
    posA.makeMove(6, 6, CellStatus::White);

    GameBoard boardB(19, Seat::First);
    SearchPosition19 posB = SearchPosition19::fromBoard(boardB);
    posB.makeMove(6, 6, CellStatus::Black);
    posB.makeMove(5, 5, CellStatus::White);

    CHECK(posA.zobristHash() != posB.zobristHash());
}

TEST_CASE("makeMove sets stone in correct bitboard plane")
{
    SearchPosition19 pos = emptyPos();

    pos.makeMove(5, 7, CellStatus::Black);
    pos.makeMove(9, 3, CellStatus::White);

    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().black, 5, 7) == true);
    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().white, 5, 7) == false);

    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().white, 9, 3) == true);
    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().black, 9, 3) == false);
}

TEST_CASE("undoMove clears stone from bitboard")
{
    SearchPosition19 pos = emptyPos();

    pos.makeMove(5, 7, CellStatus::Black);
    pos.makeMove(9, 3, CellStatus::White);

    pos.undoMove(9, 3, CellStatus::White);
    pos.undoMove(5, 7, CellStatus::Black);

    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().black, 5, 7) == false);
    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().white, 9, 3) == false);
}

// ── Capture-aware tests ───────────────────────────────────────────────────────

// Setup: B W W B  (horizontal, row 5, cols 2-5)
// Placing Black at col 5 captures the two whites at cols 3 and 4.
static SearchPosition captureSetup()
{
    GameBoard board(19, Seat::First);
    board.placeStoneOfColor(2, 5, CellStatus::Black);
    board.placeStoneOfColor(3, 5, CellStatus::White);
    board.placeStoneOfColor(4, 5, CellStatus::White);
    return SearchPosition::fromBoard(board);
}

TEST_CASE("makeMove with capture removes opponent stones from bitboard")
{
    SearchPosition pos = captureSetup();

    pos.makeMove(5, 5, CellStatus::Black);

    CHECK( get_bb19(pos.board().black, index_bb19(5, 5)));
    CHECK(!get_bb19(pos.board().white, index_bb19(3, 5)));
    CHECK(!get_bb19(pos.board().white, index_bb19(4, 5)));
    CHECK( get_bb19(pos.board().black, index_bb19(2, 5)));
}

TEST_CASE("makeMove with capture updates capture count")
{
    SearchPosition pos = captureSetup();

    CHECK(pos.captureCount(Color::Black) == 0);
    pos.makeMove(5, 5, CellStatus::Black);
    CHECK(pos.captureCount(Color::Black) == 2);
}

TEST_CASE("undoMove after capture fully restores bitboard and hash")
{
    SearchPosition pos     = captureSetup();
    uint64_t       initial = pos.zobristHash();

    pos.makeMove(5, 5, CellStatus::Black);
    pos.undoMove(5, 5, CellStatus::Black);

    CHECK(pos.zobristHash() == initial);
    CHECK(!get_bb19(pos.board().black, index_bb19(5, 5)));
    CHECK( get_bb19(pos.board().white, index_bb19(3, 5)));
    CHECK( get_bb19(pos.board().white, index_bb19(4, 5)));
    CHECK(pos.captureCount(Color::Black) == 0);
}

TEST_CASE("incremental hash matches full recompute after a capture move")
{
    SearchPosition pos = captureSetup();

    pos.makeMove(5, 5, CellStatus::Black);

    uint64_t incremental = pos.zobristHash();
    uint64_t recomputed  = SearchPosition::hasher().compute(pos.board());

    CHECK(incremental == recomputed);
}

TEST_CASE("make/undo sequence with two captures fully restores hash")
{
    // Two separate captures across two plies
    GameBoard board(19, Seat::First);
    // Ply 1: Black plays (5,5), captures whites at (3,5) and (4,5)
    board.placeStoneOfColor(2, 5, CellStatus::Black);
    board.placeStoneOfColor(3, 5, CellStatus::White);
    board.placeStoneOfColor(4, 5, CellStatus::White);
    // Ply 2 setup: White plays (10,10), captures blacks at (10,8) and (10,9)
    board.placeStoneOfColor(10, 7, CellStatus::White);
    board.placeStoneOfColor(10, 8, CellStatus::Black);
    board.placeStoneOfColor(10, 9, CellStatus::Black);

    SearchPosition pos     = SearchPosition::fromBoard(board);
    uint64_t       initial = pos.zobristHash();

    pos.makeMove(5,  5,  CellStatus::Black);
    pos.makeMove(10, 10, CellStatus::White);

    pos.undoMove(10, 10, CellStatus::White);
    pos.undoMove(5,  5,  CellStatus::Black);

    CHECK(pos.zobristHash() == initial);
    CHECK(pos.captureCount(Color::Black) == 0);
    CHECK(pos.captureCount(Color::White) == 0);
}
