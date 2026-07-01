#include "optimization/TranspositionTable.hpp"

TranspositionTable::TranspositionTable()
    : _table(TABLE_SIZE)
{}

const TTEntry* TranspositionTable::probe(uint64_t hash) const
{
    const TTEntry& entry = _table[hash & (TABLE_SIZE - 1)];

    if (entry.zobristKey != hash)
        return nullptr;

    return &entry;
}

void TranspositionTable::store(uint64_t hash, int32_t score, uint8_t depth,
                               TTFlag flag, t_cell bestMove)
{
    TTEntry& entry = _table[hash & (TABLE_SIZE - 1)];

    entry.zobristKey = hash;
    entry.score      = score;
    entry.depth      = depth;
    entry.flag       = flag;
    entry.bestMove   = bestMove;
}

void TranspositionTable::clear()
{
    _table.assign(TABLE_SIZE, TTEntry{});
}
