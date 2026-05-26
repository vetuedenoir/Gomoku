#ifndef STANDARDRULES_HPP
# define STANDARDRULES_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/BitboardTool.hpp"
#include "game/contracts/contracts.hpp"
#include "logger/Logger.hpp"

template<typename Traits>
class StandardRules
{
    public:
        bool isLegal(const t_BWBoard<Traits>& board, int col, int row, const Color color) const;
};

template<typename Traits>
bool StandardRules<Traits>::isLegal(const t_BWBoard<Traits>& board, int col, int row, const Color color) const
{
    if (!in_board_generic<Traits>(col, row))
        return false;

    if (get_bb_generic<Traits>(board.black, col, row) ||
        get_bb_generic<Traits>(board.white, col, row))
        return false;

    t_BWBoard<Traits> temp = board;
    typename Traits::Bitboard& moverPlane =
        (color == Color::Black) ? temp.black : temp.white;
    typename Traits::Bitboard& oppPlane =
        (color == Color::Black) ? temp.white : temp.black;
    set_bb_generic<Traits>(moverPlane, col, row);


    // on est oblige de faire les checks dans cet ordre pour detecter les double three, 
    // sinon is double three buggerait en detectant un simple three avant de detecter le double three
    if (BitboardTool<Traits>::instance().check_open_four(moverPlane, oppPlane, col, row) > 0)
        return true;
    if (BitboardTool<Traits>::instance().check_broken_four(moverPlane, oppPlane, col, row) > 0)
        return true;
    if (BitboardTool<Traits>::instance().is_double_three_at(moverPlane, oppPlane, col, row))
    {
        typename Traits::Bitboard captureMask = {};
        if (!detect_captures<Traits>(temp, col, row, color, captureMask))
            return false;
    }

    return true;
}

using StandardRules19 = StandardRules<BoardTraits<19>>;
using StandardRules15 = StandardRules<BoardTraits<15>>;

#endif
