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

    // Pas de copie du board : on pose le bit sur le plan du joueur (mutation en
    // place puis restauration avant de sortir). Les checks ne lisent que les deux
    // plans, et detect_captures ne modifie pas le board. Mono-thread => sur.
    typename Traits::Bitboard& moverPlane = const_cast<typename Traits::Bitboard&>(
        (color == Color::Black) ? board.black : board.white);
    const typename Traits::Bitboard& oppPlane =
        (color == Color::Black) ? board.white : board.black;

    set_bb_generic<Traits>(moverPlane, col, row);

    BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

    // on est oblige de faire les checks dans cet ordre pour detecter les double three,
    // sinon is double three buggerait en detectant un simple three avant de detecter le double three
    bool        legal = true;
    const char* why   = nullptr;
    if (tool.check_open_four(moverPlane, oppPlane, col, row) > 0
        || tool.check_broken_four(moverPlane, oppPlane, col, row) > 0)
    {
        legal = true;  // un four a priorite sur la regle du double-trois
    }
    else if (tool.is_double_three_at(moverPlane, oppPlane, col, row))
    {
        typename Traits::Bitboard captureMask = {};
        if (!detect_captures<Traits>(board, col, row, color, captureMask))
        {
            legal = false;
            why   = "double-three";
        }
    }

    clear_bit_generic<Traits>(moverPlane, col, row);  // restauration de l'etat d'origine

    if (!legal)
        return reject(why);
    return true;
}

using StandardRules19 = StandardRules<BoardTraits<19>>;
using StandardRules15 = StandardRules<BoardTraits<15>>;

#endif
