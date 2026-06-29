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
    Move     bestMove   = {-1, -1, CellStatus::Empty};
};

class TranspositionTable
{
    public:
        TranspositionTable();

        const TTEntry* probe(uint64_t hash) const;

        void           store(uint64_t hash, int32_t score, uint8_t depth,
                             TTFlag flag, Move bestMove);

        void           clear();

    private:
        static constexpr std::size_t TABLE_SIZE = 1u << 20; // 1,048,576 entries

        // Heap-backed so the table can be embedded by value (e.g. in MasterAI)
        // without risking a stack overflow, and so entries start zero-initialized.
        std::vector<TTEntry> _table;
};

#endif
