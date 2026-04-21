#include "optimization/ZobristHasher.hpp"

#define SEED 42

uint64_t ZobristHasher::_table[19 * 20][2] = {};
uint64_t ZobristHasher::_sideKey           = 0;
bool     ZobristHasher::_initialized       = false;

uint64_t random_u64() {
    static std::mt19937_64 rng(SEED);
    static std::uniform_int_distribution<uint64_t> dist;
    return dist(rng);
}

ZobristHasher::ZobristHasher(int boardSize) : _boardSize(boardSize), _stride(boardSize + 1) { }

void ZobristHasher::init()
{
    for (int y = 0; y < _boardSize; ++y)
    {
        for (int x = 0; x < _boardSize; ++x)
        {
            int i = y * _stride + x;
            _table[i][0] = random_u64();
            _table[i][1] = random_u64();
        }
    }
    _sideKey     = random_u64();
    _initialized = true;
}

uint64_t ZobristHasher::key(int x, int y, Color color) const
{
    return _table[y * _stride + x][static_cast<int>(color)];
}

uint64_t ZobristHasher::compute(const t_BWBoard19& board) const
{
    uint64_t hash = 0;

    for (int y = 0; y < _boardSize; ++y)
    {
        for (int x = 0; x < _boardSize; ++x)
        {
            const int i = y * _stride + x;

            if ((board.black[i >> 6] >> (i & 63)) & 1ULL)
                hash ^= _table[i][static_cast<int>(Color::Black)];

            if ((board.white[i >> 6] >> (i & 63)) & 1ULL)
                hash ^= _table[i][static_cast<int>(Color::White)];
        }
    }

    return hash;
}

uint64_t ZobristHasher::compute(const t_BWBoard15& board) const
{
    // TODO: same logic as the 19×19 overload — only the board type differs.
    (void)board;
    return 0;
}