#ifndef WINDETECTOR_HPP
# define WINDETECTOR_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/BitboardTool.hpp"
#include "game/contracts/contracts.hpp"


// fonction qui ne detecte pas les captures gagnantes, mais seulement les alignements de 5
// surement à compléter plus tard pour les captures
template<typename Traits>
bool isWinAfterMove(const t_BWBoard<Traits>& bb, const Color color, int col, int row)
{
	return BitboardTool<Traits>::instance().is_five_in_a_row(
		bitboardForColor(bb, color), col, row);
}

#endif
