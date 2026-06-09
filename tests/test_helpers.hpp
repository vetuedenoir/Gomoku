#pragma once

#include "bitboard/bitboard.hpp"


/* Move t_cell */
static bool contains_move(const std::vector<t_cell>& moves, int x, int y)
{
	for (const auto& m : moves)
		if (m.x == x && m.y == y)
			return true;
	return false;
}

/* Bitboard 19*/
using bitboard19 = typename BoardTraits<19>::Bitboard;

inline void set_bb19(typename BoardTraits<19>::Bitboard& bb, int x, int y) {
    set_bb_generic<BoardTraits<19>>(bb, x, y);
}

inline int index_bb19(int x, int y) {
    return index_bb_generic<BoardTraits<19>>(x, y);
}

inline bool get_bb19(const bitboard19& bb, int idx) {
    return get_bb_flate<BoardTraits<19>>(bb, idx);
}

inline int popcount_bb(const bitboard19& bb) {
    return popcount_bb_generic<BoardTraits<19>>(bb);
}


/* Bitboard 15*/
using bitboard15 = typename BoardTraits<15>::Bitboard;

inline void set_bb15(typename BoardTraits<15>::Bitboard& bb, int x, int y) {
    set_bb_generic<BoardTraits<15>>(bb, x, y);
}

inline int index_bb15(int x, int y) {
    return index_bb_generic<BoardTraits<15>>(x, y);
}

inline bool get_bb15(const bitboard15& bb, int idx) {
    return get_bb_flate<BoardTraits<15>>(bb, idx);
}

inline int popcount_bb(const bitboard15& bb) {
    return popcount_bb_generic<BoardTraits<15>>(bb);
}