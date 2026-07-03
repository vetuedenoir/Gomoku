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
        bool isLegal(const t_BWBoard<Traits>& board, int col, int row, const Color color,
                     const char** reason = nullptr) const;
};

template<typename Traits>
bool StandardRules<Traits>::isLegal(const t_BWBoard<Traits>& board, int col, int row, const Color color,
                                    const char** reason) const
{
    const auto reject = [&](const char* why) {
        if (reason)
            *reason = why;
        return false;
    };

    if (!in_board_generic<Traits>(col, row))
        return reject("out of board");

    if (get_bb_generic<Traits>(board.black, col, row) ||
        get_bb_generic<Traits>(board.white, col, row))
        return reject("cell occupied");

    /*Create a copy of the board to simulate the move*/
    t_BWBoard<Traits> simulatedBoard = board;
    typename Traits::Bitboard& moverPlane = (color == Color::Black) ? simulatedBoard.black : simulatedBoard.white;
    typename Traits::Bitboard& oppPlane = (color == Color::Black) ? simulatedBoard.white : simulatedBoard.black;
    
    /*Simulate the move*/
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
        if (!detect_captures<Traits>(simulatedBoard, col, row, color, captureMask))
            return reject("double-three");
    }

    return true;
}

using StandardRules19 = StandardRules<BoardTraits<19>>;
using StandardRules15 = StandardRules<BoardTraits<15>>;

#endif
