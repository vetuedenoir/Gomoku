#pragma once

#include "bitboard/bitboard.hpp"

using bitboard15 = typename BoardTraits<15>::Bitboard;

static GameBoard empty_board()
{
	return GameBoard(15, Color::Black);
}

static t_BWBoard15 to_bb(const GameBoard& b)
{
	return GameBoard_to_bitboard<BoardTraits<15> >(b);
}

inline void set_bb15(typename BoardTraits<15>::Bitboard& bb, int x, int y)
{
	set_bb_generic<BoardTraits<15> >(bb, x, y);
}

inline int index_bb15(int x, int y)
{
	return index_bb_generic<BoardTraits<15> >(x, y);
}

inline bool get_bb15(const bitboard15& bb, int idx)
{
	return get_bb_flate<BoardTraits<15> >(bb, idx);
}

inline int popcount_bb(const bitboard15& bb)
{
	return popcount_bb_generic<BoardTraits<15> >(bb);
}