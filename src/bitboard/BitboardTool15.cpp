#include "bitboard/BitboardTool15.hpp"

BitboardTool15::BitboardTool15()
{
    build_lookup_table5<BoardTraits<15>>(lookup_table5);
    build_lookup_table4<BoardTraits<15>>(lookup_table4);
    build_lookup_table_groupe4<BoardTraits<15>>(lookup_table_groupe4);
    build_lookup_table3<BoardTraits<15>>(lookup_table3);
    build_lookup_table_super4<BoardTraits<15>>(lookup_table_super4);
    build_lookup_table_cross<BoardTraits<15>>(lookup_table_cross);
}

BitboardTool15::~BitboardTool15()
{
}
