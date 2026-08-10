#ifndef TRANSPOSITIONTABLE_HPP
# define TRANSPOSITIONTABLE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include "game/contracts/contracts.hpp"

// How to interpret the stored score relative to the search window.
enum class TTFlag : uint8_t
{
    Exact,       // exact minimax score
    LowerBound,  // score >= beta (caused a cutoff — actual score may be higher)
    UpperBound   // score <= alpha (all moves were worse — actual score may be lower)
};

struct TTEntry
{
    uint64_t zobristKey = 0;
    int32_t  score      = 0;
    uint8_t  depth      = 0;
    TTFlag   flag       = TTFlag::Exact;
    t_cell   bestMove   = {-1, -1};
};

# define GOMOKU_TT_BITS 24
#endif

class TranspositionTable
{
    public:
        TranspositionTable();

        const TTEntry* probe(uint64_t hash) const;

        void           store(uint64_t hash, int32_t score, uint8_t depth,
                            TTFlag flag, t_cell bestMove);

        void           clear();

    private:
        static constexpr std::size_t TABLE_SIZE = std::size_t(1) << GOMOKU_TT_BITS;

        // Heap-backed so the table can be embedded by value (e.g. in MasterAI)
        // without risking a stack overflow, and so entries start zero-initialized.
        std::vector<TTEntry> _table;
};
