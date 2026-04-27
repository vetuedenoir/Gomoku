#ifndef TRANSPOSITIONTABLE_HPP
# define TRANSPOSITIONTABLE_HPP

#include <cstdint>
#include "game/contracts/Move.hpp"

// // How to interpret the stored score relative to the search window.
// enum class TTFlag : uint8_t
// {
//     Exact,       // exact minimax score
//     LowerBound,  // score >= beta (caused a cutoff — actual score may be higher)
//     UpperBound   // score <= alpha (all moves were worse — actual score may be lower)
// };

struct TTEntry
{
    uint64_t zobristKey;
    int16_t  score;
    uint8_t  depth;
    Move     bestMove;
};

class TranspositionTable
{
    private:
        static constexpr int TABLE_SIZE = 1024;
        TTEntry              _table[TABLE_SIZE];

    public:
        const TTEntry* probe(uint64_t hash) const;

        // void           store(uint64_t hash, int16_t score, uint8_t depth,
        //                      TTFlag flag, Move bestMove);

        void           store(uint64_t hash, int16_t score, uint8_t depth, Move bestMove);
};

#endif
