#ifndef SEARCHSTATE_HPP
# define SEARCHSTATE_HPP

#include <cstdint>
#include "game/board/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "optimization/ZobristHasher.hpp"
#include "bitboard/bitboard.hpp"

// TODO: rename this ? what is the relation with the search algorithm ?
template<typename Traits>
class SearchState
{
public:
    static SearchState fromBoard(const GameBoard& src);
    static const ZobristHasher<Traits>& hasher();

    void makeMove(int col, int row, CellStatus color);
    void undoMove(int col, int row, CellStatus color);

    const t_BWBoard<Traits>& board()       const;
    uint64_t                 zobristHash() const;
    Color                    sideToMove()  const;

private:
    SearchState(t_BWBoard<Traits> board, uint64_t hash, Color side,
                const ZobristHasher<Traits>* hasher);

    t_BWBoard<Traits>              _board;
    uint64_t                       _hash;
    Color                          _sideToMove;
    const ZobristHasher<Traits>*   _hasher; // TODO: why a pointer ?
};

template<typename Traits>
static Color toColor(CellStatus s)
{
    return (s == CellStatus::Black) ? Color::Black : Color::White;
}

template<typename Traits>
static typename Traits::Bitboard& plane(t_BWBoard<Traits>& board, CellStatus color)
{
    return (color == CellStatus::Black) ? board.black : board.white;
}

template<typename Traits>
SearchState<Traits>::SearchState(t_BWBoard<Traits> board, uint64_t hash, Color side,
                                 const ZobristHasher<Traits>* h)
    : _board(board), _hash(hash), _sideToMove(side), _hasher(h)
{}

template<typename Traits>
SearchState<Traits> SearchState<Traits>::fromBoard(const GameBoard& src)
{
    const ZobristHasher<Traits>& h = SearchState<Traits>::hasher();
    t_BWBoard<Traits> bb   = GameBoard_to_bitboard<Traits>(src);
    uint64_t          hash = h.compute(bb);
    Color             side = (src.currentSeat() == Seat::First) ? Color::Black : Color::White;
    return SearchState<Traits>(bb, hash, side, &h);
}

template<typename Traits>
const ZobristHasher<Traits>& SearchState<Traits>::hasher()
{
    return ZobristHasher<Traits>::instance();
}

template<typename Traits>
void SearchState<Traits>::makeMove(int col, int row, CellStatus color)
{
    set_bb_generic<Traits>(plane<Traits>(_board, color), col, row);
    _hash ^= _hasher->key(col, row, toColor<Traits>(color));
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
}

template<typename Traits>
void SearchState<Traits>::undoMove(int col, int row, CellStatus color)
{
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
    _hash ^= _hasher->key(col, row, toColor<Traits>(color));
    clear_bit_generic<Traits>(plane<Traits>(_board, color), col, row);
}

template<typename Traits>
const t_BWBoard<Traits>& SearchState<Traits>::board() const
{
    return _board;
}

template<typename Traits>
uint64_t SearchState<Traits>::zobristHash() const
{
    return _hash;
}

template<typename Traits>
Color SearchState<Traits>::sideToMove() const
{
    return _sideToMove;
}

using SearchState19 = SearchState<BoardTraits<19>>;
using SearchState15 = SearchState<BoardTraits<15>>;

#endif
