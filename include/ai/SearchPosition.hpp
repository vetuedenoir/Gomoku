#ifndef SEARCHSTATE_HPP
# define SEARCHSTATE_HPP

#include <cstdint>
#include "game/board/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "optimization/ZobristHasher.hpp"
#include "bitboard/bitboard.hpp"

// a "SearchPosition" conventionally includes:
// - board occupancy,
// - side to move,
// - hash,
// - reversible incremental mutations.
template<typename Traits>
class SearchPosition
{
public:
    static SearchPosition fromBoard(const GameBoard& src);
    static const ZobristHasher<Traits>& hasher();

    void makeMove(int col, int row, CellStatus color);
    void undoMove(int col, int row, CellStatus color);

    const t_BWBoard<Traits>& board()       const;
    uint64_t                 zobristHash() const;
    Color                    sideToMove()  const;

private:
    SearchPosition(t_BWBoard<Traits> board, uint64_t hash, Color side,
                const ZobristHasher<Traits>& hasher);

    t_BWBoard<Traits>              _board;
    uint64_t                       _hash;
    Color                          _sideToMove;
    const ZobristHasher<Traits>&   _hasher;
};

template<typename Traits>
static Color toColor(CellStatus s)
{
    return (s == CellStatus::Black) ? Color::Black : Color::White;
}

template<typename Traits>
SearchPosition<Traits>::SearchPosition(t_BWBoard<Traits> board, uint64_t hash, Color side,
                                 const ZobristHasher<Traits>& hasher)
    : _board(board), _hash(hash), _sideToMove(side), _hasher(hasher)
{}

template<typename Traits>
SearchPosition<Traits> SearchPosition<Traits>::fromBoard(const GameBoard& src)
{
    const ZobristHasher<Traits>& h = SearchPosition<Traits>::hasher();
    t_BWBoard<Traits> bb   = GameBoard_to_bitboard<Traits>(src);
    uint64_t          hash = h.compute(bb);
    Color             side = (src.currentSeat() == Seat::First) ? Color::Black : Color::White;
    return SearchPosition<Traits>(bb, hash, side, h);
}

template<typename Traits>
const ZobristHasher<Traits>& SearchPosition<Traits>::hasher()
{
    return ZobristHasher<Traits>::instance();
}

template<typename Traits>
void SearchPosition<Traits>::makeMove(int col, int row, CellStatus color)
{
    set_bb_generic<Traits>(bitboardForColor(_board, toColor<Traits>(color)), col, row);
    _hash ^= _hasher.key(col, row, toColor<Traits>(color));
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
}

template<typename Traits>
void SearchPosition<Traits>::undoMove(int col, int row, CellStatus color)
{
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
    _hash ^= _hasher.key(col, row, toColor<Traits>(color));
    clear_bit_generic<Traits>(bitboardForColor(_board, toColor<Traits>(color)), col, row);
}

template<typename Traits>
const t_BWBoard<Traits>& SearchPosition<Traits>::board() const
{
    return _board;
}

template<typename Traits>
uint64_t SearchPosition<Traits>::zobristHash() const
{
    return _hash;
}

template<typename Traits>
Color SearchPosition<Traits>::sideToMove() const
{
    return _sideToMove;
}

using SearchPosition19 = SearchPosition<BoardTraits<19>>;
using SearchPosition15 = SearchPosition<BoardTraits<15>>;
using SearchState19    = SearchPosition19;
using SearchState15    = SearchPosition15;

#endif
