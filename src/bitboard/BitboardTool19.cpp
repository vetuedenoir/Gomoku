#include "bitboard/BitboardTool19.hpp"

BitboardTool19::BitboardTool19()
{
    build_lookup_table5<BoardTraits<19>>(lookup_table5);
    build_lookup_table4<BoardTraits<19>>(lookup_table4);
    build_lookup_table_groupe4<BoardTraits<19>>(lookup_table_groupe4);
    build_lookup_table3<BoardTraits<19>>(lookup_table3);
    build_lookup_table_super4<BoardTraits<19>>(lookup_table_super4);
    build_lookup_table_cross<BoardTraits<19>>(lookup_table_cross);
}

BitboardTool19::~BitboardTool19()
{
}

