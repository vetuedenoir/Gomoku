#ifndef ZOBRISTHASHER_HPP
# define ZOBRISTHASHER_HPP

#include <array>
#include <cstdint>
#include <random>
#include "game/contracts/Color.hpp"
#include "bitboard.hpp"

// Static Zobrist lookup table.
//
// Usage:
//   1. Call ZobristHasher::init() once at program startup.
//   2. Call ZobristHasher::compute() to build a hash from a full position.
//   3. Call ZobristHasher::key() to get the incremental XOR value for one
//      (x, y, color) triple — use for make/unmake inside the AI search.
//
// Table layout:
//   _table[squareIdx][colorIdx]
//   squareIdx = y * 20 + x   (matches bitboard19 stride, 1-bit row padding)
//   colorIdx  = 0 (Black) | 1 (White)
//
// Square count: 19 rows × 20 stride = 380 entries per color plane.

class ZobristHasher
{
    private:
        static uint64_t _table[19 * 20][2];
        static uint64_t _sideKey;
        static bool     _initialized;

        int _boardSize;
        int _stride;  // = boardSize + 1, matches the bitboard row stride

    public:
        explicit ZobristHasher(int boardSize);

        void     init();

        uint64_t key(int x, int y, Color color) const;

        // Computes a full hash from scratch by iterating the set bits of both
        // bitboard planes. Used in SearchPosition::fromBoard().
        // Equivalent to XOR-ing key() for every stone currently on the board.
        uint64_t compute(const t_BWBoard19& board) const;
        uint64_t compute(const t_BWBoard15& board) const;
};

#endif
