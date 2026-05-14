#ifndef SEARCHPOSITION_HPP
# define SEARCHPOSITION_HPP

#include <cstdint>
#include <vector>
#include "game/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "optimization/ZobristHasher.hpp"
#include "bitboard/bitboard.hpp"

// AI's mutable board state for use during search.
//
// Owns a t_BWBoard<Traits> bitboard and a Zobrist hash updated incrementally
// via makeMove / undoMove (XOR is self-inverse, so undo == the same XOR).
//
// fromBoard() is the sole entry point from the UI layer — it converts once
// at the root of each AI turn and the engine stays in bitboard space.

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
                       const ZobristHasher<Traits>* hasher);

        t_BWBoard<Traits>          _board;
        uint64_t                   _hash;
        Color                      _sideToMove;
        const ZobristHasher<Traits>* _hasher;
};

// ── Implementation ────────────────────────────────────────────────────────────

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
SearchPosition<Traits>::SearchPosition(t_BWBoard<Traits> board, uint64_t hash,
                                       Color side, const ZobristHasher<Traits>* h)
    : _board(board), _hash(hash), _sideToMove(side), _hasher(h)
{}

template<typename Traits>
SearchPosition<Traits> SearchPosition<Traits>::fromBoard(const GameBoard& src)
{
    const ZobristHasher<Traits>& h = SearchPosition<Traits>::hasher();
    t_BWBoard<Traits> bb   = GameBoard_to_bitboard<Traits>(src);
    uint64_t          hash = h.compute(bb);
    Color             side = (src.currentSeat() == Seat::First) ? Color::Black : Color::White;
    return SearchPosition<Traits>(bb, hash, side, &h);
}

template<typename Traits>
const ZobristHasher<Traits>& SearchPosition<Traits>::hasher()
{
    return ZobristHasher<Traits>::instance();
}

template<typename Traits>
void SearchPosition<Traits>::makeMove(int col, int row, CellStatus color)
{
    set_bb_generic<Traits>(plane<Traits>(_board, color), col, row);
    _hash ^= _hasher->key(col, row, toColor<Traits>(color));
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
}

template<typename Traits>
void SearchPosition<Traits>::undoMove(int col, int row, CellStatus color)
{
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
    _hash ^= _hasher->key(col, row, toColor<Traits>(color));
    clear_bit_generic<Traits>(plane<Traits>(_board, color), col, row);
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

// ── Convenience aliases ───────────────────────────────────────────────────────

using SearchPosition19 = SearchPosition<BoardTraits<19>>;
using SearchPosition15 = SearchPosition<BoardTraits<15>>;

#endif // SEARCHPOSITION_HPP
