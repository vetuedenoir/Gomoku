#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/GameBoard.hpp"

#include <cstdint>
#include <iostream>
#include <bitset>
#include <string.h>


// ── bitboard ─────────────────────────────────────────────────────────────────

#include "game/contracts/Color.hpp"


#define	MAX_WINNING_MASK	1020


typedef uint64_t bitboard19[6];


typedef struct s_BWBoard19 {
	bitboard19 black;
	bitboard19 white;
}   t_BWBoard19;

#define DIR_HORIZ 0
#define DIR_VERT  1
#define DIR_DIAG_G 2  //
#define DIR_DIAG_D 3  //

#define IDX(x, y) ((y) * 19 + (x))


typedef struct {
	uint64_t masks[5][6];  // 5 masks × 6 uint64_t = 240 bytes
	uint8_t count;          // 1 byte (au lieu de int)
	uint8_t padding[7];
} MaskList;  // 248 bytes total



typedef struct {
	uint64_t masks[4][6]; //4 masks × 6 uint64_t = 240 bytes
	int  left_pos[4];	
	int  right_pos[4];
	uint8_t count;         // 1 byte (au lieu de int)
} MaskList4;  // 257 bytes total



// il y a un padding de 1 bit entre les lignes, donc on peut faire 20x20 = 400 cases 
// avec 6 uint64_t (384 bits) + 16 bits de padding
// inline int index(int x, int y);

inline int index(int x, int y) {
	return y * 20 + x;
}

// set le bit a 1 a la position x, y
inline void set(bitboard19 &bb, int x, int y);

inline bool get(const bitboard19& bb, int i);

void print_bb_19(t_BWBoard19 &bb);

t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board);



// testing
void	test_bitboard(const GameBoard &board, int x, int y);

#endif // BITBOARD_HPP