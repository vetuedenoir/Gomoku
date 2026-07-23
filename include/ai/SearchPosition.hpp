#ifndef SEARCHSTATE_HPP
# define SEARCHSTATE_HPP

#include <cstdint>
#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "optimization/ZobristHasher.hpp"
#include "bitboard/bitboard.hpp"
#include "config/config.hpp"


// a "SearchPosition" conventionally includes:
// - board occupancy,
// - side to move,
// - hash,
// - reversible incremental mutations.
struct MoveState {
    int      blackCaptures;
    int      whiteCaptures;
    MoveList   <t_cell, 16> capturedStones; // PIERRES RETIRÉES (à restaurer)
};

struct MoveStateHash
{
    uint64_t		hash;
    MoveState		state;
};


template<typename Traits>
class SearchPosition
{
    
    private:
        SearchPosition(t_BWBoard<Traits> board, uint64_t hash, const Color side,
            const ZobristHasher<Traits>& hasher);
        

        int                            _blackCaptures;
        int                            _whiteCaptures;
        t_BWBoard<Traits>              _board;
        uint64_t                       _hash;
        Color                          _sideToMove;
        const ZobristHasher<Traits>&   _hasher;
        
        MoveList<MoveState, DEPTH>	_history; // stock tout utile pour remetre les pierres captures sur le board

    public:
        static SearchPosition fromBoard(const GameBoard& src);
        static const ZobristHasher<Traits>& hasher();
    
        // int detect_captures_hash(const t_BWBoard<Traits>& board, int col, int row, 
        //     const Color attackerColor, typename Traits::Bitboard& capturedMask);

        int detect_captures_hash(const t_BWBoard<Traits>& board, int col, int row, 
            const Color attackerColor, std::array<t_cell, 16>& capturedStones);
        

        
        // void makeMove(int col, int row, Color color);
        void undoMove(int col, int row, Color color);
    
        int  getTotalblackCaptures() const { return _blackCaptures; }
        int  getTotalwhiteCaptures() const { return _whiteCaptures; }

        int getCapturesForside() const
        {
            return (sideToMove() == Color::Black) ? getTotalblackCaptures() : getTotalwhiteCaptures();
        }

        int getCapturesForColor(Color color) const
        {
            return (color == Color::Black) ? getTotalwhiteCaptures() : getTotalblackCaptures();
        }

        int getBlackCaptures() const { return _history.empty() ? 0 : _history.top().blackCaptures; }
        int getWhiteCaptures() const { return _history.empty() ? 0 : _history.top().whiteCaptures; }
    
        const t_BWBoard<Traits>& board()       const;
        uint64_t                 zobristHash() const;
        Color                    sideToMove()  const;

        MoveStateHash buildMoveHash(EvaluatedMove move, Color color) const;

        int detect_and_hash_capture(const t_BWBoard<Traits>& board, int col, int row, const Color attackerColor, MoveStateHash& stateHash) const;
        
        void makeMove(int col, int row, Color color, MoveStateHash stateHash);
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
{
    _whiteCaptures = 0;
    _blackCaptures = 0;
}

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
int SearchPosition<Traits>::detect_captures_hash(const t_BWBoard<Traits>& board, int col, int row, const Color attackerColor, std::array<t_cell, 16>& capturedStones)
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
                clear_bit_generic<Traits>(bitboardForColor(const_cast<t_BWBoard<Traits>&>(board), victimColor), x1, y1);
                clear_bit_generic<Traits>(bitboardForColor(const_cast<t_BWBoard<Traits>&>(board), victimColor), x2, y2);
                _hash ^= _hasher.key(x1, y1, victimColor);
                _hash ^= _hasher.key(x2, y2, victimColor);
                capturedStones[captured] = {x1, y1};
                capturedStones[captured + 1] = {x2, y2};
				captured += 2;
			}
		}
	}
	return captured;
}



// template<typename Traits>
// void SearchPosition<Traits>::makeMove(int col, int row, Color color)
// {
//     set_bb_generic<Traits>(bitboardForColor(_board, color), col, row);
//     _hash ^= _hasher.key(col, row, color);
//     _hash ^= _hasher.sideKey();


//     MoveState state  = {};
//     int caps = detect_captures_hash(_board, col, row, color, state.capturedStones);
//     if (caps > 0)
//     {
//         if (color == Color::Black) // couleur de l'attaquant
//         {
//             state.whiteCaptures = caps;
//             _whiteCaptures += caps;
//             _hash ^= _hasher.captureHash(Color::White, _whiteCaptures >> 1);
//         }
//         else
//         {
//             state.blackCaptures = caps;
//             _blackCaptures += caps;
//             _hash ^= _hasher.captureHash(Color::Black, _blackCaptures >> 1);
//         }
//     }
//     _history.push_back(state);
//     _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
// }

template<typename Traits>
void SearchPosition<Traits>::undoMove(int col, int row, Color color)
{
    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
    _hash ^= _hasher.sideKey(); 
    _hash ^= _hasher.key(col, row, color);

    MoveState state = _history.top();
    _history.pop();

    if (state.whiteCaptures > 0)
	{
		_hash ^= _hasher.captureHash(Color::White, _whiteCaptures >> 1);
		_whiteCaptures -= state.whiteCaptures;
		// Restaure uniquement les vraies pierres capturees (borne au compteur).
		for (int k = 0; k < state.whiteCaptures; ++k)
		{
			set_bb_generic<Traits>(_board.white, state.capturedStones[k].x, state.capturedStones[k].y);
			_hash ^= _hasher.key(state.capturedStones[k].x, state.capturedStones[k].y, Color::White);
		}
	}	
    else if (state.blackCaptures > 0)
	{
		_hash ^= _hasher.captureHash(Color::Black, _blackCaptures >> 1);
		_blackCaptures -= state.blackCaptures;
		for (int k = 0; k < state.blackCaptures; ++k)
		{
			set_bb_generic<Traits>(_board.black, state.capturedStones[k].x, state.capturedStones[k].y);
			_hash ^= _hasher.key(state.capturedStones[k].x, state.capturedStones[k].y, Color::Black);
		}
    }

    clear_bit_generic<Traits>(bitboardForColor(_board, color), col, row);
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


template<typename Traits>
void SearchPosition<Traits>::makeMove(int col, int row, Color color, MoveStateHash stateHash)
{
    set_bb_generic<Traits>(bitboardForColor(_board, color), col, row);
    if (stateHash.state.whiteCaptures > 0)
    {
        _whiteCaptures += stateHash.state.whiteCaptures;
        for (int k = 0; k < stateHash.state.whiteCaptures; ++k)
            clear_bit_generic<Traits>(_board.white, stateHash.state.capturedStones[k].x, stateHash.state.capturedStones[k].y);
    }
    else if (stateHash.state.blackCaptures > 0)
    {
        _blackCaptures += stateHash.state.blackCaptures;
        for (int k = 0; k < stateHash.state.blackCaptures; ++k)
            clear_bit_generic<Traits>(_board.black, stateHash.state.capturedStones[k].x, stateHash.state.capturedStones[k].y);
    }

    _history.push(stateHash.state);

    _hash = stateHash.hash;

    _sideToMove = (_sideToMove == Color::Black) ? Color::White : Color::Black;
}


template<typename Traits>
int SearchPosition<Traits>::detect_and_hash_capture(const t_BWBoard<Traits>& board, int col, int row, const Color attackerColor, MoveStateHash& stateHash) const
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
                stateHash.hash ^= _hasher.key(x1, y1, victimColor);
                stateHash.hash ^= _hasher.key(x2, y2, victimColor);
                stateHash.state.capturedStones[captured] = {x1, y1};
                stateHash.state.capturedStones[captured + 1] = {x2, y2};
				captured += 2;
			}
		}
	}
	return captured;
}


// template<typename Traits>
// MoveStateHash SearchPosition<Traits>::buildMoveHash(int col, int row, Color color) const
// {
//     MoveStateHash stateHash = {};

//     stateHash.hash = _hash;
//     stateHash.hash ^= _hasher.key(col, row, color);
//     stateHash.hash ^= _hasher.sideKey();

    
//     int caps = detect_and_hash_capture(_board, col, row, color, stateHash);
//     if (caps > 0)
//     {
//         if (color == Color::Black)
//         {
//             stateHash.state.whiteCaptures = caps;
//             stateHash.hash ^= _hasher.captureHash(Color::White, (_whiteCaptures + caps) >> 1);
//         }
//         else
//         {
//             stateHash.state.blackCaptures = caps;
//             stateHash.hash ^= _hasher.captureHash(Color::Black, (_blackCaptures + caps) >> 1);
//         }
//     }
//     return stateHash;
// }


template<typename Traits>
MoveStateHash SearchPosition<Traits>::buildMoveHash(EvaluatedMove move, Color color) const
{
    MoveStateHash stateHash = {};

    stateHash.hash = _hash;
    stateHash.hash ^= _hasher.key(move.move.x, move.move.y, color);
    stateHash.hash ^= _hasher.sideKey();

    
    int caps = move.capturedStones.size();
    if (caps > 0)
    {
        stateHash.state.capturedStones = move.capturedStones;
        
        const Color victimColor = (color == Color::Black) ? Color::White : Color::Black;
        for (size_t k = 0; k < move.capturedStones.size(); ++k)
        {
            const t_cell& stone = move.capturedStones[k];
            stateHash.hash ^= _hasher.key(stone.x, stone.y, victimColor);
        }

        if (color == Color::Black)
        {
            stateHash.state.whiteCaptures = caps;
            stateHash.hash ^= _hasher.captureHash(Color::White, (_whiteCaptures + caps) >> 1);
        }
        else
        {
            stateHash.state.blackCaptures = caps;
            stateHash.hash ^= _hasher.captureHash(Color::Black, (_blackCaptures + caps) >> 1);
        }
    }
    return stateHash;
}


using SearchPosition19 = SearchPosition<BoardTraits<19>>;
using SearchPosition15 = SearchPosition<BoardTraits<15>>;
using SearchState19    = SearchPosition19;
using SearchState15    = SearchPosition15;

#endif
