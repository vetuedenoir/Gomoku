#ifndef WINDETECTOR_HPP
# define WINDETECTOR_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/BitboardTool.hpp"
#include "game/contracts/contracts.hpp"

template<typename Traits>
bool isWinAfterMove(const t_BWBoard<Traits>& bb, const Color color, int col, int row)
{
	return BitboardTool<Traits>::instance().is_five_in_a_row(
		bitboardForColor(bb, color), col, row);
}

#endif
