#ifndef MOVEGENERATOR_HPP
# define MOVEGENERATOR_HPP

#include "ai/ActiveZone.hpp"
#include "bitboard/bitboard.hpp"
#include "game/validation/rules/StandardRules.hpp"
#include <algorithm>


template<typename Traits>
class MoveGenerator
{
	public:
		explicit MoveGenerator(int activeZoneRadius = 2);

		BitboardTool<Traits> getBitboardTool() const;

		// This mask can be used to generate a list of legal moves by iterating over the bits in the mask.
		void getMaskOfLegalMoves(const t_BWBoard<Traits>& board, const Color color,
								typename Traits::Bitboard& legalMovesMask) const;

		std::vector<t_cell> generateMoves(const t_BWBoard<Traits>& board, const Color color) const;
		size_t	generateMovesT(const t_BWBoard<Traits>& board, const Color color, std::array<t_cell, 200>& movesArray) const;

		bool isLegalMove(const t_BWBoard<Traits>& board, int col, int row, const Color color) const;

	private:
		int                 _activeZoneRadius;
		StandardRules<Traits> _standardRules;
};

template<typename Traits>
MoveGenerator<Traits>::MoveGenerator(int activeZoneRadius)
	: _activeZoneRadius(activeZoneRadius)
{}

template<typename Traits>
bool MoveGenerator<Traits>::isLegalMove(const t_BWBoard<Traits>& board,
										int col, int row, const Color color) const
{
	return _standardRules.isLegal(board, col, row, color);
}

template<typename Traits>
void MoveGenerator<Traits>::getMaskOfLegalMoves(const t_BWBoard<Traits>& board, const Color color,
                                               typename Traits::Bitboard& legalMovesMask) const
{
	legalMovesMask = {};
    ActiveZone<Traits> zone(_activeZoneRadius);
    zone.initialize(board);
    const typename Traits::Bitboard& mask = zone.getCandidateMask();

    for (int y = 0; y < Traits::BOARD_SIZE; ++y)
    {
        for (int x = 0; x < Traits::BOARD_SIZE; ++x)
        {
            if (get_bb_generic<Traits>(mask, x, y) && isLegalMove(board, x, y, color))
                set_bb_generic<Traits>(legalMovesMask, x, y);
        }
    }
}


template<typename Traits>
std::vector<t_cell>	MoveGenerator<Traits>::generateMoves(const t_BWBoard<Traits>& board, const Color color) const
{
	ActiveZone<Traits> zone(_activeZoneRadius);
	zone.initialize(board);

	std::vector<t_cell> zoneMoves = zone.generateZoneMoves();

	for (auto it = zoneMoves.begin(); it != zoneMoves.end(); )
	{
		if (!isLegalMove(board, it->x, it->y, color))
			it = zoneMoves.erase(it);
		else
			++it;
	}

	return zoneMoves;
}

template<typename Traits>
size_t MoveGenerator<Traits>::generateMovesT(const t_BWBoard<Traits>& board, const Color color, std::array<t_cell, 200>& movesArray) const
{
    ActiveZone<Traits> zone(_activeZoneRadius);
    zone.initialize(board);

    std::array<t_cell, 200> tmpMoves;
    size_t tmpCount = zone.generateZoneMovesT(tmpMoves);

    size_t moveCount = 0;

    for (size_t i = 0; i < tmpCount; ++i)
    {
        if (isLegalMove(board, tmpMoves[i].x, tmpMoves[i].y, color))
        {
            movesArray[moveCount++] = tmpMoves[i];
        }
    }

    return moveCount;
}

using MoveGenerator19 = MoveGenerator<BoardTraits<19>>;
using MoveGenerator15 = MoveGenerator<BoardTraits<15>>;

#endif
