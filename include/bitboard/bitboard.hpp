#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/GameBoard.hpp"

#include <cstdint>
#include <iostream>
#include <bitset>
#include <string.h>


// ── bitboard ─────────────────────────────────────────────────────────────────

#include "game/contracts/Color.hpp"

typedef uint64_t bitboard19[6];

typedef struct s_BWBoard19 {
	bitboard19 black;
	bitboard19 white;
}   t_BWBoard19;


inline int	index_bb19(int x, int y)
{
	return y * 20 + x;
}

inline void	set_bb19(bitboard19 &bb, int x, int y)
{
	int idx = index_bb19(x, y);
	bb[idx / 64] |= (1ULL << (idx % 64));
}

inline bool	get_bb19(const bitboard19& bb, int i)
{
	int idx = index_bb19(i % 20, i / 20);
	return (bb[idx / 64] & (1ULL << (idx % 64))) != 0;
}

t_BWBoard19	GameBoard_to_bitboard(const GameBoard &board);


void	print_bb_19(t_BWBoard19 &bb);
void	print_binaire64(const uint64_t ut);
void	print_binaire_board19(const bitboard19 bb);


#endif // BITBOARD_HPP