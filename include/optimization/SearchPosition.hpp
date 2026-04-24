#ifndef SEARCHPOSITION_HPP
# define SEARCHPOSITION_HPP

#include <cstdint>
#include "game/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "optimization/ZobristHasher.hpp"

// SearchPosition is the AI's private, mutable copy of the board.
//
// It is the sole owner of the board during a search — the UI's GameBoard is
// never touched. make/undoMove maintain the Zobrist hash incrementally using
// a single XOR (self-inverse, so undo == make for the hash update).
//
// TEMPORARY: the board is represented as a GameBoard (CellStatus[19][19]).
// Future migration: replace with t_BWBoard19 bitboards. The public interface
// (makeMove / undoMove / board() / zobristHash()) stays unchanged.
//
// Lifetime rule: the ZobristHasher passed to fromBoard() must outlive this
// object. In practice it is initialised at startup and lives forever.

class SearchPosition
{
    public:
        // Copies src and computes the starting hash from scratch.
        // Call once at the root of each AI turn.
        static SearchPosition fromBoard(const GameBoard& src, const ZobristHasher& hasher);

        // Place a stone of the given color; update hash; switch current player.
        void makeMove(int col, int row, CellStatus color);

        // Erase the stone placed by makeMove; XOR hash back; switch player.
        // Precondition: (col, row, color) must match the last makeMove call.
        void undoMove(int col, int row, CellStatus color);

        const GameBoard& board()       const;
        uint64_t         zobristHash() const;

    private:
        SearchPosition(GameBoard board, uint64_t hash, const ZobristHasher* hasher);

        GameBoard            _board;
        uint64_t             _hash;
        const ZobristHasher* _hasher;  // non-owning borrow
};

#endif
