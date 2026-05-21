#pragma once

#include "bitboard/bitboard.hpp"

using TestTraits = BoardTraits<19>;

using bitboard19 = typename TestTraits::Bitboard;

inline void set_bb19(typename TestTraits::Bitboard& bb, int x, int y) {
    set_bb_generic<TestTraits>(bb, x, y);
}

inline int index_bb19(int x, int y) {
    return index_bb_generic<TestTraits>(x, y);
}

inline bool get_bb19(const bitboard19& bb, int idx) {
    return get_bb_flate<TestTraits>(bb, idx);
}

inline int popcount_bb(const bitboard19& bb) {
    return popcount_bb_generic<TestTraits>(bb);
}
