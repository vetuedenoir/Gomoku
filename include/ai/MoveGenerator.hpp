#ifndef MOVEGENERATOR_HPP
# define MOVEGENERATOR_HPP

#include "ai/ActiveZone.hpp"
#include "bitboard/bitboard.hpp"
#include "bitboard/pattern.hpp"
#include "game/rules/StandardRules.hpp"

template<typename Traits>
class MoveGenerator
{
    public:
        explicit MoveGenerator(int activeZoneRadius = 2);

        // Fills `legalMoves` with all empty candidate cells that pass the legality check for `color`. 
        void generateLegalMoves(const t_BWBoard<Traits>& board, Color color,
                                typename Traits::Bitboard& legalMoves) const;

        bool isLegalMove(const t_BWBoard<Traits>& board, int col, int row, Color color) const;

    private:
        int                 _activeZoneRadius;
        StandardRules<Traits> _standardRules;
};

template<typename Traits>
MoveGenerator<Traits>::MoveGenerator(int activeZoneRadius)
    : _activeZoneRadius(activeZoneRadius)
{}

template<typename Traits>
void MoveGenerator<Traits>::generateLegalMoves(const t_BWBoard<Traits>& board,
                                               Color color,
                                               typename Traits::Bitboard& legalMoves) const
{
    legalMoves = {};

    ActiveZone<Traits> zone(_activeZoneRadius);
    zone.initialize(board);

    const typename Traits::Bitboard& candidates = zone.getCandidateMask();

    for (int y = 0; y < Traits::BOARD_SIZE; y++)
    {
        for (int x = 0; x < Traits::BOARD_SIZE; x++)
        {
            if (get_bb_generic<Traits>(candidates, x, y) && isLegalMove(board, x, y, color))
                set_bb_generic<Traits>(legalMoves, x, y);
        }
    }
}

template<typename Traits>
bool MoveGenerator<Traits>::isLegalMove(const t_BWBoard<Traits>& board,
                                        int col, int row, Color color) const
{
    return _standardRules.isLegal(board, col, row, color);
}

using MoveGenerator19 = MoveGenerator<BoardTraits<19>>;
using MoveGenerator15 = MoveGenerator<BoardTraits<15>>;

#endif
