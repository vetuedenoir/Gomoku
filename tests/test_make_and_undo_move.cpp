#include "doctest.h"
#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "ai/SearchPosition.hpp"
#include "bitboard/bitboard.hpp"

static SearchPosition19 emptyPos()
{
    GameBoard board(19, Color::Black);
    return SearchPosition19::fromBoard(board);
}

// makeMove now takes a precomputed MoveStateHash; build it first from the
// current position, then apply.
static void doMove(SearchPosition19& pos, int col, int row, Color color)
{
    pos.makeMove(col, row, color, pos.buildMoveHash(col, row, color));
}

TEST_CASE("make/undo single move restores hash")
{
    SearchPosition19 pos     = emptyPos();
    uint64_t         initial = pos.zobristHash();

    doMove(pos, 10, 10, Color::Black);
    pos.undoMove(10, 10, Color::Black);

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

    doMove(pos, 5, 5, Color::Black);
    doMove(pos, 6, 6, Color::White);
    doMove(pos, 7, 7, Color::Black);

    pos.undoMove(7, 7, Color::Black);
    pos.undoMove(6, 6, Color::White);
    pos.undoMove(5, 5, Color::Black);

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

    doMove(pos, 3,  3,  Color::Black);
    doMove(pos, 10, 10, Color::White);

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
    GameBoard boardA(19, Color::Black);
    SearchPosition19 posA = SearchPosition19::fromBoard(boardA);
    doMove(posA, 5, 5, Color::Black);
    doMove(posA, 6, 6, Color::White);

    GameBoard boardB(19, Color::White);
    SearchPosition19 posB = SearchPosition19::fromBoard(boardB);
    doMove(posB, 6, 6, Color::Black);
    doMove(posB, 5, 5, Color::White);

    CHECK(posA.zobristHash() != posB.zobristHash());
}

TEST_CASE("makeMove sets stone in correct bitboard plane")
{
    SearchPosition19 pos = emptyPos();

    doMove(pos, 5, 7, Color::Black);
    doMove(pos, 9, 3, Color::White);

    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().black, 5, 7) == true);
    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().white, 5, 7) == false);

    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().white, 9, 3) == true);
    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().black, 9, 3) == false);
}

TEST_CASE("undoMove clears stone from bitboard")
{
    SearchPosition19 pos = emptyPos();

    doMove(pos, 5, 7, Color::Black);
    doMove(pos, 9, 3, Color::White);

    pos.undoMove(9, 3, Color::White);
    pos.undoMove(5, 7, Color::Black);

    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().black, 5, 7) == false);
    CHECK(get_bb_generic<BoardTraits<19>>(pos.board().white, 9, 3) == false);
}

// Capture-aware make/undo lives in TurnController / GameController, not SearchPosition.
