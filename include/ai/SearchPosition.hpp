#ifndef SEARCHSTATE_HPP
# define SEARCHSTATE_HPP

#include <cstdint>
#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
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
    
    private:
        SearchPosition(t_BWBoard<Traits> board, uint64_t hash, const Color side,
            const ZobristHasher<Traits>& hasher);
        
        struct MoveState {
            uint64_t hash;
            Color    sideToMove;
            int      blackCaptures;
            int      whiteCaptures;
            typename Traits::Bitboard capturedStones; // PIERRES RETIRÉES (à restaurer)
        };

        int                            _blackCaptures;
        int                            _whiteCaptures;
        t_BWBoard<Traits>              _board;
        uint64_t                       _hash;
        Color                          _sideToMove;
        const ZobristHasher<Traits>&   _hasher;
        
        std::vector<MoveState>         _history; // stock tout utile pour remetre les pierres captures sur le board

    public:
        static SearchPosition fromBoard(const GameBoard& src);
        static const ZobristHasher<Traits>& hasher();
    
        int detect_captures_hash(const t_BWBoard<Traits>& board, int col, int row, 
            const Color attackerColor, typename Traits::Bitboard& capturedMask);
        
        void makeMove(int col, int row, CellStatus color);
        void undoMove(int col, int row, CellStatus color);
    
        int  blackCaptures() const { return _blackCaptures; }
        int  whiteCaptures() const { return _whiteCaptures; }
    
        const t_BWBoard<Traits>& board()       const;
        uint64_t                 zobristHash() const;
        Color                    sideToMove()  const;
        
};

template<typename Traits>
static Color toColor(CellStatus s)
{
    return (s == CellStatus::Black) ? Color::Black : Color::White;
}

template<typename Traits>
SearchPosition<Traits>::SearchPosition(t_BWBoard<Traits> board, uint64_t hash, const Color side,
                                 const ZobristHasher<Traits>& hasher)
    : _board(board), _hash(hash), _sideToMove(side), _hasher(hasher)
{}

template<typename Traits>
SearchPosition<Traits> SearchPosition<Traits>::fromBoard(const GameBoard& src)
{
    const ZobristHasher<Traits>& h = SearchPosition<Traits>::hasher();
    t_BWBoard<Traits> bb   = GameBoard_to_bitboard<Traits>(src);
    uint64_t          hash = h.compute(bb);
    Color             side = src.currentColor();
    return SearchPosition<Traits>(bb, hash, side, h);
}

template<typename Traits>
const ZobristHasher<Traits>& SearchPosition<Traits>::hasher()
{
    return ZobristHasher<Traits>::instance();
}


template<typename Traits>
int SearchPosition<Traits>::detect_captures_hash(const t_BWBoard<Traits>& board, int col, int row, const Color attackerColor, typename Traits::Bitboard& capturedMask)
{
	const typename Traits::Bitboard& attacker = bitboardForColor(board, attackerColor);
	const Color victimColor = (attackerColor == Color::Black) ? Color::White : Color::Black;
	const typename Traits::Bitboard& victime = bitboardForColor(board, victimColor);

	int captured = 0;

	for (Direction dir : LINE_DIRS)
	{
		for (int sign : {-1, 1})
		{
			int stepX = sign * dx(dir);
			int stepY = sign * dy(dir);

			int x1 = col + 1 * stepX, y1 = row + 1 * stepY;
			int x2 = col + 2 * stepX, y2 = row + 2 * stepY;
			int x3 = col + 3 * stepX, y3 = row + 3 * stepY;

			if (!in_board_generic<Traits>(x1, y1) || !in_board_generic<Traits>(x2, y2) || !in_board_generic<Traits>(x3, y3))
				continue;

			if (get_bb_generic<Traits>(victime, x1, y1) &&
				get_bb_generic<Traits>(victime, x2, y2) &&
				get_bb_generic<Traits>(attacker, x3, y3))
			{
                _hash ^= _hasher.key(x1, y1, attackerColor);
                _hash ^= _hasher.key(x2, y2, attackerColor);
				set_bb_generic<Traits>(capturedMask, x1, y1);
				set_bb_generic<Traits>(capturedMask, x2, y2);
				captured += 2;
			}
		}
	}

	return captured;
}

template<typename Traits>
void SearchPosition<Traits>::makeMove(int col, int row, CellStatus color)
{
    set_bb_generic<Traits>(bitboardForColor(_board, toColor<Traits>(color)), col, row);
    _hash ^= _hasher.key(col, row, toColor<Traits>(color));
    _hash ^= _hasher.sideKey();


    MoveState state  = {};
    
    int caps = detect_captures_hash(_board, col, row, toColor<Traits>(color), state.capturedStones);

    if (toColor<Traits>(color) == Color::Black)
        _whiteCaptures += caps;
    else
        _blackCaptures += caps;

    state.hash          = _hash;
    state.sideToMove    = _sideToMove;
    state.blackCaptures = _blackCaptures;
    state.whiteCaptures = _whiteCaptures;

    _history.push_back(state);
        
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
}

template<typename Traits>
void SearchPosition<Traits>::undoMove(int col, int row, CellStatus color)
{
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
    _hash ^= _hasher.sideKey();
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
