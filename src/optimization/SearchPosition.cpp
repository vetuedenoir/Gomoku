#include "optimization/SearchPosition.hpp"

static Color toColor(CellStatus s)
{
    return (s == CellStatus::Black) ? Color::Black : Color::White;
}

static bitboard19& plane(t_BWBoard19& board, CellStatus color)
{
    return (color == CellStatus::Black) ? board.black : board.white;
}

static ZobristHasher& sharedHasher()
{
    static ZobristHasher h(19);
    static bool          ready = false;

    if (!ready)
    {
        h.init();
        ready = true;
    }
    return h;
}

SearchPosition::SearchPosition(t_BWBoard19 board, uint64_t hash, Color side, const ZobristHasher* hasher)
    : _board(board), _hash(hash), _sideToMove(side), _hasher(hasher)
{}

SearchPosition SearchPosition::fromBoard(const GameBoard& src)
{
    const ZobristHasher& hasher = SearchPosition::hasher();
    t_BWBoard19 bb   = GameBoard_to_bitboard(src);
    uint64_t    hash = hasher.compute(bb);
    Color       side = (src.currentSeat() == Seat::First) ? Color::Black : Color::White;
    return SearchPosition(bb, hash, side, &hasher);
}

const ZobristHasher& SearchPosition::hasher()
{
    return sharedHasher();
}

void SearchPosition::makeMove(int col, int row, CellStatus color)
{
    set(plane(_board, color), col, row);
    _hash ^= _hasher->key(col, row, toColor(color));
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
}

void SearchPosition::undoMove(int col, int row, CellStatus color)
{
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
    _hash ^= _hasher->key(col, row, toColor(color));
    clear_bit(plane(_board, color), col, row);
}

const t_BWBoard19& SearchPosition::board() const
{
    return _board;
}

uint64_t SearchPosition::zobristHash() const
{
    return _hash;
}

Color SearchPosition::sideToMove() const
{
    return _sideToMove;
}
