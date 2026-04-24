#include "doctest.h"
#include "game/GameBoard.hpp"
#include "game/Seat.hpp"
#include "optimization/SearchPosition.hpp"
#include "optimization/ZobristHasher.hpp"
#include "bitboard.hpp"

// The hasher's static table is filled once and shared across all tests.
static ZobristHasher& hasher()
{
    static ZobristHasher h(19);
    static bool          ready = false;
    if (!ready) { h.init(); ready = true; }
    return h;
}

static SearchPosition emptyPos()
{
    GameBoard board(19, Seat::First);
    return SearchPosition::fromBoard(board, hasher());
}

// ── Test 1: make then undo restores hash ─────────────────────────────────────
//
// The most fundamental invariant: undoMove must be the perfect inverse of
// makeMove. If this fails the hash is broken, period.

TEST_CASE("make/undo single move restores hash")
{
    SearchPosition pos     = emptyPos();
    uint64_t       initial = pos.zobristHash();

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
    SearchPosition pos     = emptyPos();
    uint64_t       initial = pos.zobristHash();

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
    SearchPosition pos = emptyPos();

    pos.makeMove(3,  3,  CellStatus::Black);
    pos.makeMove(10, 10, CellStatus::White);

    uint64_t    incremental = pos.zobristHash();
    t_BWBoard19 bb          = GameBoard_to_bitboard(pos.board());
    uint64_t    recomputed  = hasher().compute(bb);

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
    SearchPosition posA = SearchPosition::fromBoard(boardA, hasher());
    posA.makeMove(5, 5, CellStatus::Black);
    posA.makeMove(6, 6, CellStatus::White);

    GameBoard boardB(19, Seat::First);
    SearchPosition posB = SearchPosition::fromBoard(boardB, hasher());
    posB.makeMove(6, 6, CellStatus::Black);
    posB.makeMove(5, 5, CellStatus::White);

    CHECK(posA.zobristHash() != posB.zobristHash());
}
