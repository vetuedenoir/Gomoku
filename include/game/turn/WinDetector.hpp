#ifndef WINDETECTOR_HPP
# define WINDETECTOR_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/pattern.hpp"
#include "game/contracts/contracts.hpp"
#include <cstring>

template<typename Traits>
bool isWinAfterMove(const t_BWBoard<Traits>& bb, const Color color, int col, int row)
{
    static t_PatternList5<Traits> table[Traits::BOARD_SIZE * Traits::BOARD_SIZE][4];
    static bool                     ready = false;
    if (!ready)
    {
        std::memset(table, 0, sizeof(table));
        build_lookup_table5<Traits>(table);
        ready = true;
    }
    const auto& stonesBb = bitboardForColor(bb, color);
    return isWin_ultra<Traits>(table, stonesBb, col, row) > 0;
}

#endif
