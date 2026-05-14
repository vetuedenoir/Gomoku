#include "bitboard/BitboardTool15.hpp"

BitboardTool15::BitboardTool15()
{
    build_lookup_table5(lookup_table5);
    build_lookup_table4(lookup_table4);
    build_lookup_table_groupe4(lookup_table_groupe4);
    build_lookup_table3(lookup_table3);
    build_lookup_table_super4(lookup_table_super4);
    build_lookup_table_cross(lookup_table_cross);
}

BitboardTool15::~BitboardTool15()
{
}
