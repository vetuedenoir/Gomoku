#ifndef ZOBRISTHASHER_HPP
# define ZOBRISTHASHER_HPP

#include <array>
#include <cstdint>
#include <random>
#include "game/contracts/contracts.hpp"
#include "bitboard/bitboard.hpp"

// Static Zobrist lookup table, parameterized on board traits.
//
// Usage:
//   1. Call ZobristHasher<Traits>::instance() to get the singleton (auto-inits).
//   2. Call hasher.compute(board) to build a hash from a full position.
//   3. Call hasher.key(x, y, color) for the incremental XOR value of one move.
//
// Table layout:
//   _table[y * Traits::STRIDE + x][colorIdx]
//   colorIdx = 0 (Black) | 1 (White)
//
// Each Traits specialization gets its own static table and _initialized flag,
// so ZobristHasher<BoardTraits<15>> and ZobristHasher<BoardTraits<19>> are
// fully independent.

#define ZOBRIST_SEED 42

template<typename Traits>
class ZobristHasher
{
    static constexpr int TABLE_SIZE = Traits::BOARD_SIZE * Traits::STRIDE;

    static uint64_t _table[TABLE_SIZE][2];
    static uint64_t _sideKey;
    static bool     _initialized;

    static uint64_t random_u64()
    {
        static std::mt19937_64 rng(ZOBRIST_SEED);
        static std::uniform_int_distribution<uint64_t> dist;
        return dist(rng);
    }

    ZobristHasher()
    {
        if (_initialized)
            return;
        for (int y = 0; y < Traits::BOARD_SIZE; ++y)
        {
            for (int x = 0; x < Traits::BOARD_SIZE; ++x)
            {
                int i = y * Traits::STRIDE + x;
                _table[i][0] = random_u64();
                _table[i][1] = random_u64();
            }
        }
        _sideKey     = random_u64();
        _initialized = true;
    }

public:
    static const ZobristHasher& instance()
    {
        static ZobristHasher h;
        return h;
    }

    uint64_t key(int x, int y, const Color color) const
    {
        return _table[y * Traits::STRIDE + x][static_cast<int>(color)];
    }

    uint64_t sideKey() const
    {
        return _sideKey;
    }

    uint64_t compute(const t_BWBoard<Traits>& board) const
    {
        uint64_t hash = 0;

        for (int y = 0; y < Traits::BOARD_SIZE; ++y)
        {
            for (int x = 0; x < Traits::BOARD_SIZE; ++x)
            {
                const int i = y * Traits::STRIDE + x;

                if (get_bb_generic<Traits>(board.black, x, y))
                    hash ^= _table[i][static_cast<int>(Color::Black)];

                if (get_bb_generic<Traits>(board.white, x, y))
                    hash ^= _table[i][static_cast<int>(Color::White)];
            }
        }

        return hash;
    }
};

template<typename Traits> uint64_t ZobristHasher<Traits>::_table[ZobristHasher<Traits>::TABLE_SIZE][2] = {};
template<typename Traits> uint64_t ZobristHasher<Traits>::_sideKey = 0;
template<typename Traits> bool     ZobristHasher<Traits>::_initialized = false;

using ZobristHasher19 = ZobristHasher<BoardTraits<19>>;
using ZobristHasher15 = ZobristHasher<BoardTraits<15>>;

#endif // ZOBRISTHASHER_HPP
