#ifndef SEARCHSTATE_HPP
# define SEARCHSTATE_HPP

#include <cstdint>
#include <array>
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

        static constexpr int ZONE_RADIUS = ACTIVE_ZONE_RADIUS;

        typename Traits::Bitboard                                _zoneMask {};
        std::array<uint8_t, Traits::STRIDE * Traits::BOARD_SIZE> _zoneCount {};

        void zoneApplyStone(int cx, int cy, int delta);
        void zoneRebuild();

    public:
        // capturesByBlack / capturesByWhite = stones each colour has taken
        // (GameController tally). Mapped internally to victim counters.
        static SearchPosition fromBoard(const GameBoard& src,
                                        int capturesByBlack = 0,
                                        int capturesByWhite = 0);
        
        static const ZobristHasher<Traits>& hasher();
    
        void undoMove(int col, int row, Color color);
    
        int  getTotalblackCaptures() const { return _blackCaptures; }
        int  getTotalwhiteCaptures() const { return _whiteCaptures; }

        // Stones taken BY `color` (Black's score lives in _whiteCaptures, etc.).
        int getCapturesForColor(Color color) const
        {
            return (color == Color::Black) ? getTotalwhiteCaptures() : getTotalblackCaptures();
        }

        // Stones taken by the side about to move.
        int getCapturesForside() const
        {
            return getCapturesForColor(sideToMove());
        }

        int getBlackCaptures() const { return _history.empty() ? 0 : _history.top().blackCaptures; }
        int getWhiteCaptures() const { return _history.empty() ? 0 : _history.top().whiteCaptures; }

        void    setBlackCaptures(int count) { _blackCaptures = count; }
        void    setWhiteCaptures(int count) { _whiteCaptures = count; }

    
        const t_BWBoard<Traits>& board()       const;
        uint64_t                 zobristHash() const;
        Color                    sideToMove()  const;

        typename Traits::Bitboard candidateMask() const;

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
    zoneRebuild();
}

// Applique le voisinage (boîte de rayon ZONE_RADIUS) d'une pierre posée (delta=+1)
// ou retirée (delta=-1) sur les compteurs, et met à jour _zoneMask aux transitions
// 0->1 (allumage) et 1->0 (extinction).
template<typename Traits>
void SearchPosition<Traits>::zoneApplyStone(int cx, int cy, int delta)
{
    for (int dy = -ZONE_RADIUS; dy <= ZONE_RADIUS; ++dy)
    {
        for (int dx = -ZONE_RADIUS; dx <= ZONE_RADIUS; ++dx)
        {
            const int nx = cx + dx;
            const int ny = cy + dy;
            if (!in_board_generic<Traits>(nx, ny))
                continue;

            const int idx = index_bb_generic<Traits>(nx, ny);
            if (delta > 0)
            {
                if (++_zoneCount[idx] == 1)
                    set_bb_generic<Traits>(_zoneMask, nx, ny);
            }
            else
            {
                if (--_zoneCount[idx] == 0)
                    clear_bit_generic<Traits>(_zoneMask, nx, ny);
            }
        }
    }
}

// Recalcule intégralement compteurs + masque depuis _board. Utilisé une seule
// fois à la construction ; ensuite l'entretien est purement incrémental.
template<typename Traits>
void SearchPosition<Traits>::zoneRebuild()
{
    _zoneMask = {};
    _zoneCount.fill(0);
    bb_for_each_bit<Traits>(_board.black, [&](int x, int y){ zoneApplyStone(x, y, +1); });
    bb_for_each_bit<Traits>(_board.white, [&](int x, int y){ zoneApplyStone(x, y, +1); });
}

template<typename Traits>
typename Traits::Bitboard SearchPosition<Traits>::candidateMask() const
{
    typename Traits::Bitboard out;
    bool anyStone = false;
    for (int i = 0; i < Traits::WORD_COUNT; ++i)
    {
        const uint64_t occ = _board.black[i] | _board.white[i];
        out[i] = _zoneMask[i] & ~occ;
        anyStone = anyStone || (occ != 0);
    }
    if (!anyStone) // plateau vide -> on amorce le centre (parité avec ActiveZone)
        set_bb_generic<Traits>(out, Traits::BOARD_SIZE / 2, Traits::BOARD_SIZE / 2);
    return out;
}

template<typename Traits>
SearchPosition<Traits> SearchPosition<Traits>::fromBoard(const GameBoard& src,
                                                        int capturesByBlack,
                                                        int capturesByWhite)
{
    const ZobristHasher<Traits>& h = SearchPosition<Traits>::hasher();
    t_BWBoard<Traits> bb   = GameBoard_to_bitboard<Traits>(src);
    uint64_t          hash = h.compute(bb);
    Color             side = src.currentColor();

    // Match make/undo convention: XOR pair-count keys 1..N for each victim colour.
    // SearchPosition stores victim tallies: Black's score → _whiteCaptures.
    const int whiteVictimPairs = capturesByBlack >> 1;
    const int blackVictimPairs = capturesByWhite >> 1;
    for (int p = 1; p <= whiteVictimPairs; ++p)
        hash ^= h.captureHash(Color::White, p);
    for (int p = 1; p <= blackVictimPairs; ++p)
        hash ^= h.captureHash(Color::Black, p);

    SearchPosition<Traits> pos(bb, hash, side, h);
    pos._whiteCaptures = capturesByBlack;
    pos._blackCaptures = capturesByWhite;
    return pos;
}

template<typename Traits>
const ZobristHasher<Traits>& SearchPosition<Traits>::hasher()
{
    return ZobristHasher<Traits>::instance();
}

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
			zoneApplyStone(state.capturedStones[k].x, state.capturedStones[k].y, +1);
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
			zoneApplyStone(state.capturedStones[k].x, state.capturedStones[k].y, +1);
			_hash ^= _hasher.key(state.capturedStones[k].x, state.capturedStones[k].y, Color::Black);
		}
    }

    clear_bit_generic<Traits>(bitboardForColor(_board, color), col, row);
    zoneApplyStone(col, row, -1);
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
    zoneApplyStone(col, row, +1);
    if (stateHash.state.whiteCaptures > 0)
    {
        _whiteCaptures += stateHash.state.whiteCaptures;
        for (int k = 0; k < stateHash.state.whiteCaptures; ++k)
        {
            clear_bit_generic<Traits>(_board.white, stateHash.state.capturedStones[k].x, stateHash.state.capturedStones[k].y);
            zoneApplyStone(stateHash.state.capturedStones[k].x, stateHash.state.capturedStones[k].y, -1);
        }
    }
    else if (stateHash.state.blackCaptures > 0)
    {
        _blackCaptures += stateHash.state.blackCaptures;
        for (int k = 0; k < stateHash.state.blackCaptures; ++k)
        {
            clear_bit_generic<Traits>(_board.black, stateHash.state.capturedStones[k].x, stateHash.state.capturedStones[k].y);
            zoneApplyStone(stateHash.state.capturedStones[k].x, stateHash.state.capturedStones[k].y, -1);
        }
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
                stateHash.state.capturedStones[captured] = {static_cast<int_fast16_t>(x1), static_cast<int_fast16_t>(y1)};
                stateHash.state.capturedStones[captured + 1] = {static_cast<int_fast16_t>(x2), static_cast<int_fast16_t>(y2)};
				captured += 2;
			}
		}
	}
	return captured;
}

template<typename Traits>
MoveStateHash SearchPosition<Traits>::buildMoveHash(EvaluatedMove move, Color color) const
{
    MoveStateHash stateHash = {};

    stateHash.hash = _hash;
    stateHash.hash ^= _hasher.key(move.move.x, move.move.y, color);
    stateHash.hash ^= _hasher.sideKey();

    
    const int caps = capture_mask_count(move.captureMask);
    if (caps > 0)
    {
        // Les victimes ne sont reconstruites qu'ici, pour le coup effectivement
        // joué : l'ordonnancement, lui, n'a manipulé que le masque.
        const Color victimColor = (color == Color::Black) ? Color::White : Color::Black;
        for_each_captured_stone(move.captureMask, move.move.x, move.move.y, [&](int x, int y) {
            stateHash.state.capturedStones.push({static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y)});
            stateHash.hash ^= _hasher.key(x, y, victimColor);
        });

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

#endif
