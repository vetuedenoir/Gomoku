#pragma once

#include "bitboard/pattern.hpp"
#include "bitboard/bitboard.hpp"

#include <cstring>

using TestTraits = BoardTraits<19>;

// ─── Shared lookup tables, built once for the entire test run ────────────────

struct PatternTables {
    t_PatternList5<TestTraits>        lt5[TestTraits::CELL_COUNT][4];
    t_PatternList4<TestTraits>        lt4[TestTraits::CELL_COUNT][4];
    t_PatternList_Groupe4<TestTraits> ltg4[TestTraits::CELL_COUNT][4];
    t_PatternList_Groupe3             lt3[TestTraits::CELL_COUNT][4];
    t_PatternList_super4<TestTraits>  lts4[TestTraits::CELL_COUNT][4];
    t_PatternList_Cross               ltcross[TestTraits::CELL_COUNT];

    PatternTables() {
        std::memset(lt5,     0, sizeof(lt5));
        std::memset(lt4,     0, sizeof(lt4));
        std::memset(ltg4,    0, sizeof(ltg4));
        std::memset(lt3,     0, sizeof(lt3));
        std::memset(lts4,    0, sizeof(lts4));
        std::memset(ltcross, 0, sizeof(ltcross));
        build_lookup_table5<TestTraits>(lt5);
        build_lookup_table4<TestTraits>(lt4);
        build_lookup_table_groupe4<TestTraits>(ltg4);
        build_lookup_table3<TestTraits>(lt3);
        build_lookup_table_super4<TestTraits>(lts4);
        build_lookup_table_cross<TestTraits>(ltcross);
    }

    static PatternTables& get() {
        static PatternTables instance;
        return instance;
    }
};

inline t_BWBoard19 empty_bb() { return t_BWBoard19{}; }

inline void set_bb19(typename TestTraits::Bitboard& bb, int x, int y) {
    set_bb_generic<TestTraits>(bb, x, y);
}

inline int index_bb19(int x, int y) {
    return index_bb_generic<TestTraits>(x, y);
}

inline bool get_bb19(const typename TestTraits::Bitboard& bb, int idx) {
    return get_bb_flate<TestTraits>(bb, idx);
}

inline int popcount_bb(const typename TestTraits::Bitboard& bb) {
    return popcount_bb_generic<TestTraits>(bb);
}

inline int isWin_ultra_19(t_PatternList5<TestTraits> table[TestTraits::CELL_COUNT][4],
                          const typename TestTraits::Bitboard& board, int x, int y) {
    return isWin_ultra<TestTraits>(table, board, x, y);
}

inline int is_Open_4_19(t_PatternList4<TestTraits> table[TestTraits::CELL_COUNT][4],
                        const typename TestTraits::Bitboard& boardA,
                        const typename TestTraits::Bitboard& boardB, int x, int y) {
    return is_Open_4<TestTraits>(table, boardA, boardB, x, y);
}

inline int check_four_align_19(t_PatternList_Groupe4<TestTraits> table[TestTraits::CELL_COUNT][4],
                               const typename TestTraits::Bitboard& boardA,
                               const typename TestTraits::Bitboard& boardB, int x, int y) {
    return check_four_align<TestTraits>(table, boardA, boardB, x, y);
}

inline int check_three_align_19(const t_PatternList_Groupe3 table[TestTraits::CELL_COUNT][4],
                                const typename TestTraits::Bitboard& boardA,
                                const typename TestTraits::Bitboard& boardB, int x, int y) {
    return check_three_align<TestTraits>(table, boardA, boardB, x, y);
}

inline int check_super4_19(t_PatternList_super4<TestTraits> table[TestTraits::CELL_COUNT][4],
                           const typename TestTraits::Bitboard& boardA,
                           const typename TestTraits::Bitboard& boardB, int x, int y) {
    return check_super4<TestTraits>(table, boardA, boardB, x, y);
}

inline int check_cross_19(const t_PatternList_Cross table[TestTraits::CELL_COUNT],
                          const typename TestTraits::Bitboard& boardA,
                          const typename TestTraits::Bitboard& boardB, int x, int y) {
    return check_cross<TestTraits>(table, boardA, boardB, x, y);
}
