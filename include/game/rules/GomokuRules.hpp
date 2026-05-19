#ifndef GOMOKURULES_HPP
# define GOMOKURULES_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/pattern.hpp"
#include "game/contracts/Color.hpp"

template<typename Traits>
class GomokuRules
{
    public:
        bool isLegal(const t_BWBoard<Traits>& board, int col, int row, Color color) const;
};

template<typename Traits>
bool GomokuRules<Traits>::isLegal(const t_BWBoard<Traits>& board, int col, int row, Color color) const
{
    if (!in_board_generic<Traits>(col, row))
        return false;

    if (get_bb_generic<Traits>(board.black, col, row) ||
        get_bb_generic<Traits>(board.white, col, row))
        return false;

    if (is_double_three_move<Traits>(board, col, row, color))
    {
        t_BWBoard<Traits> temp = board;
        typename Traits::Bitboard& moverPlane =
            (color == Color::Black) ? temp.black : temp.white;
        set_bb_generic<Traits>(moverPlane, col, row);
        typename Traits::Bitboard captureMask = {};
        if (!detect_captures<Traits>(temp, col, row, color, captureMask))
            return false;
    }

    return true;
}

using GomokuRules19 = GomokuRules<BoardTraits<19>>;
using GomokuRules15 = GomokuRules<BoardTraits<15>>;

#endif
