#ifndef BITBOARDTOOL15_HPP 
#define BITBOARDTOOL15_HPP

#include "bitboard/BitboardTool.hpp"

// changer pour pattern15 quand il sera implemente
#include "bitboard/pattern.hpp"

class   BitboardTool15 : public IBitboardTool
{
    t_PatternList5	lookup_table5[361][4];
	t_PatternList4	lookup_table4[361][4];
	t_PatternList_Groupe4 lookup_table_groupe4[361][4];
	t_PatternList_Groupe3 lookup_table3[361][4];
	t_PatternList_super4 lookup_table_super4[361][4];
	t_PatternList_Cross lookup_table_cross[361];

public:
    BitboardTool15();
    ~BitboardTool15();


    // Implémentation des méthodes.
    // Ces méthodes utiliseront les fonctions de pattern.hpp pour vérifier les patterns sur le bitboard.
    // ou bien elles les remplaceront.
    // bool is_five_in_a_row(const GameBoard& board, const int col, const int row);
    // bool is_open_four(const GameBoard& board, const int col, const int row);
    // bool is_broken_four(const GameBoard& board, const int col, const int row);
    // bool is_open_three(const GameBoard& board, const int col, const int row);
    // bool is_broken_three(const GameBoard& board, const int col, const int row);
    // bool is_cross(const GameBoard& board, const int col, const int row);

};

#endif