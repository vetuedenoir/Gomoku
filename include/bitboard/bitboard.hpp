#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "direction.hpp"

#include <cstdint>
#include <iostream>

// template<int SIZE>
// template de toute les structures de données et fonctions pour les bitboards, pour faciliter l'adaptation à différentes tailles de plateau (15x15, 19x19, etc.)
template<int SIZE>
struct BoardTraits;

template<>
struct BoardTraits<15>
{
    static constexpr int BOARD_SIZE = 15;
    static constexpr int STRIDE = 16;
    static constexpr int CELL_COUNT = 225;
    static constexpr int WORD_COUNT = 4;

    using Bitboard = std::array<uint64_t, 4>;
};

template<>
struct BoardTraits<19>
{
    static constexpr int BOARD_SIZE = 19;
    static constexpr int STRIDE = 20;
    static constexpr int CELL_COUNT = 361;
    static constexpr int WORD_COUNT = 6;

    using Bitboard = std::array<uint64_t, 6>;
};

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


template<int STRIDE>
inline int index_bb(int x, int y)
{
    return y * STRIDE + x;
}

// utilisation: 
// int idx = index_bb<20>(x, y);
// int idx = index_bb<16>(x, y);

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

inline void clear_bit(bitboard19 &bb, int x, int y)
{
    int idx = index_bb19(x, y);
    bb[idx >> 6] &= ~(1ULL << (idx & 63));
}

inline bool in_board(int x, int y)
{
	return x >= 0 && x < 19 && y >= 0 && y < 19;
}

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


#endif // BITBOARD_HPP