#ifndef SEARCHPOSITION_HPP
# define SEARCHPOSITION_HPP

#include <cstdint>
#include "game/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "optimization/ZobristHasher.hpp"
#include "bitboard.hpp"

// AI's mutable board state for use during search.
//
// Owns a t_BWBoard19 bitboard and a Zobrist hash updated incrementally via
// makeMove / undoMove (XOR is self-inverse, so undo == the same XOR).
//
// fromBoard() is the sole entry point from the UI layer — it converts once
// at the root of each AI turn and the engine stays in bitboard space.
//
// Lifetime rule: the ZobristHasher passed to fromBoard() must outlive this object.

class SearchPosition
{
    public:
        static SearchPosition fromBoard(const GameBoard& src, const ZobristHasher& hasher);

        void makeMove(int col, int row, CellStatus color);
        void undoMove(int col, int row, CellStatus color);

        const t_BWBoard19& board()       const;
        uint64_t           zobristHash() const;
        Color              sideToMove()  const;

    private:
        SearchPosition(t_BWBoard19 board, uint64_t hash, Color side, const ZobristHasher* hasher);

        t_BWBoard19          _board;
        uint64_t             _hash;
        Color                _sideToMove;
        const ZobristHasher* _hasher;
};

#endif
