#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "direction.hpp"

#include <cstdint>
#include <iostream>




typedef uint64_t bitboard19[6];

typedef uint64_t bitboard15[4];

typedef struct s_BWBoard19 {
	bitboard19 black;
	bitboard19 white;
}   t_BWBoard19;

typedef struct s_BWBoard15 {
	bitboard15 black;
	bitboard15 white;
}   t_BWBoard15;



// il y a un padding de 1 bit entre les lignes, donc on peut faire 20x20 = 400 cases 
// avec 6 uint64_t (384 bits) + 16 bits de padding
inline int index(int x, int y) { return y * 20 + x; }

inline void set(bitboard19 &bb, int x, int y)
{
    int idx = index(x, y);
    bb[idx >> 6] |= (1ULL << (idx & 63));
}

inline bool get(const bitboard19& bb, int i) { return (bb[i >> 6] >> (i & 63)) & 1ULL; }

inline void clear_bit(bitboard19 &bb, int x, int y)
{
    int idx = index(x, y);
    bb[idx >> 6] &= ~(1ULL << (idx & 63));
}

inline bool in_board(int x, int y) { return x >= 0 && x < 19 && y >= 0 && y < 19; }

// counts set bits across all 6 words (uint64_t)
inline int popcount_bb(const bitboard19& bb)
{
    int n = 0;
    for (int i = 0; i < 6; i++) n += __builtin_popcountll(bb[i]);
    return n;
}

void        print_bb_19(t_BWBoard19 &bb);
void        print_bb_19_colored(t_BWBoard19 &bw);

t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board);

// Writes into `capturedMask` the bitmask of opponent stones flanked by placing `attacker` at (col, row).
// All four directions are checked. `capturedMask` must be zeroed by the caller.
// Returns true if at least one capture was found.
bool detect_captures(const t_BWBoard19& board, int col, int row, Color attacker, bitboard19& capturedMask);

// Removes the bits in `captured` from the victim's color board.
void apply_captures(t_BWBoard19& board, const bitboard19 captured, Color attacker);

// testing
void test_bitboard(const GameBoard &board, int x, int y);

#endif // BITBOARD_HPP