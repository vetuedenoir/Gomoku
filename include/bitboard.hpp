#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/GameBoard.hpp"

#include <cstdint>
#include <iostream>
#include <bitset>


// ── bitboard ─────────────────────────────────────────────────────────────────

enum class Color { Black, White };




typedef uint64_t bitboard19[6];


typedef struct s_BWBoard19 {
	bitboard19 black;
	bitboard19 white;
}   t_BWBoard19;



// il y a un padding de 1 bit entre les lignes, donc on peut faire 20x20 = 400 cases 
// avec 6 uint64_t (384 bits) + 16 bits de padding
inline int index(int x, int y);


// set le bit a 1 a la position x, y
inline void set(bitboard19 &bb, int x, int y);

inline bool get(const bitboard19& bb, int i);

void print_bb_19(t_BWBoard19 &bb);

t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board);



// testing
void	test_bitboard(const GameBoard &board, int x, int y);

#endif // BITBOARD_HPP