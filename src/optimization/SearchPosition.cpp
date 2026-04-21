#include "optimization/SearchPosition.hpp"
#include "bitboard.hpp"

// Converts CellStatus to the Color enum used by ZobristHasher.
// Only called with Black or White — Empty is never hashed.
static Color toColor(CellStatus s)
{
    return (s == CellStatus::Black) ? Color::Black : Color::White;
}

// ── private constructor ──────────────────────────────────────────────────────

SearchPosition::SearchPosition(GameBoard board, uint64_t hash, const ZobristHasher* hasher)
    : _board(board), _hash(hash), _hasher(hasher)
{}

// ── public interface ─────────────────────────────────────────────────────────

SearchPosition SearchPosition::fromBoard(const GameBoard& src, const ZobristHasher& hasher)
{
    t_BWBoard19 bb   = GameBoard_to_bitboard(src);
    uint64_t    hash = hasher.compute(bb);
    return SearchPosition(src, hash, &hasher);
}

void SearchPosition::makeMove(int col, int row, CellStatus color)
{
    _board.placeStoneOfColor(col, row, color);
    _hash ^= _hasher->key(col, row, toColor(color));
    _board.switchPlayer();
}

void SearchPosition::undoMove(int col, int row, CellStatus color)
{
    _board.switchPlayer();
    _hash ^= _hasher->key(col, row, toColor(color));
    _board.clearCell(col, row);
}

const GameBoard& SearchPosition::board() const
{
    return _board;
}

uint64_t SearchPosition::zobristHash() const
{
    return _hash;
}
