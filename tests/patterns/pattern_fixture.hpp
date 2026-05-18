#pragma once

#include "bitboard/pattern.hpp"
#include "bitboard/bitboard.hpp"

#include <string.h>

// ─── Shared lookup tables, built once for the entire test run ────────────────
//
// All six lookup tables total ~2.7 MB. Using a function-local static (C++11
// magic-static) ensures they are initialised exactly once regardless of how
// many translation units include this header.

struct PatternTables {
    t_PatternList5        lt5[361][4];
    t_PatternList4        lt4[361][4];
    t_PatternList_Groupe4 ltg4[361][4];
    t_PatternList_Groupe3 lt3[361][4];
    t_PatternList_super4  lts4[361][4];
    t_PatternList_Cross   ltcross[361];

    PatternTables() {
        memset(lt5,     0, sizeof(lt5));
        memset(lt4,     0, sizeof(lt4));
        memset(ltg4,    0, sizeof(ltg4));
        memset(lt3,     0, sizeof(lt3));
        memset(lts4,    0, sizeof(lts4));
        memset(ltcross, 0, sizeof(ltcross));
        build_lookup_table5(lt5);
        build_lookup_table4(lt4);
        build_lookup_table_groupe4(ltg4);
        build_lookup_table3(lt3);
        build_lookup_table_super4(lts4);
        build_lookup_table_cross(ltcross);
    }

    static PatternTables& get() {
        static PatternTables instance;
        return instance;
    }
};

inline t_BWBoard19 empty_bb() { t_BWBoard19 b = {}; return b; }
