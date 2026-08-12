#pragma once

#include "bitboard/bitboard.hpp"

/* Bitboard 19*/
using bitboard19 = typename BoardTraits<19>::Bitboard;

static GameBoard empty_board()
{
    return GameBoard(19, Color::Black);
}

static t_BWBoard19 to_bb(const GameBoard& b)
{
    return GameBoard_to_bitboard<BoardTraits<19>>(b);
}

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
