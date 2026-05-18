#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/GameBoard.hpp"
#include "game/contracts/Color.hpp"
#include "direction.hpp"

#include <cstdint>
#include <iostream>

#define BLACK_STONE "\033[1;30m"   // bright black (gray)
#define WHITE_STONE "\033[1;37m"   // bright white
#define EMPTY_CELL  "\033[2;37m"   // dim white
#define RESET   "\033[0m"

// template<int SIZE>
// template de toute les structures de données et fonctions pour les bitboards, pour faciliter l'adaptation à différentes tailles de plateau (15x15, 19x19, etc.)
template<int SIZE>
struct BoardTraits;

template<>
struct BoardTraits<15>
{
    static constexpr int BOARD_SIZE = 15;
    static constexpr int STRIDE = 16;
	static constexpr int STRIDE_G = 15;
    static constexpr int STRIDE_D = 17;
    static constexpr int CELL_COUNT = 225;
    static constexpr int WORD_COUNT = 4;

    using Bitboard = std::array<uint64_t, 4>;
};

template<>
struct BoardTraits<19>
{
    static constexpr int BOARD_SIZE = 19;
    static constexpr int STRIDE = 20;
    static constexpr int STRIDE_G = 19;
    static constexpr int STRIDE_D = 21;
    static constexpr int CELL_COUNT = 361;
    static constexpr int WORD_COUNT = 6;

    using Bitboard = std::array<uint64_t, 6>;
};

// typedef uint64_t bitboard19[6];
// typedef uint64_t bitboard15[4];

template<typename Traits>
struct t_BWBoard
{
    typename Traits::Bitboard black;
    typename Traits::Bitboard white;
};

typedef t_BWBoard<BoardTraits<19>>     t_BWBoard19;
typedef t_BWBoard<BoardTraits<15>>     t_BWBoard15;


// utilisation: 
// int idx = index_bb<20>(x, y);
// int idx = index_bb<16>(x, y);

// inline int	index_bb19(int x, int y)
// {
// 	return y * 20 + x;
// }

// inline void	set_bb19(bitboard19 &bb, int x, int y)
// {
// 	int idx = index_bb19(x, y);
// 	bb[idx / 64] |= (1ULL << (idx % 64));
// }

// inline bool	get_bb19(const bitboard19& bb, int i)
// {
// 	int idx = index_bb19(i % 20, i / 20);
// 	return (bb[idx / 64] & (1ULL << (idx % 64))) != 0;
// }

template<typename Traits>
inline int index_bb_generic(int x, int y)
{
    return y * Traits::STRIDE + x;
}


template<typename Traits>
inline int idx_generic(int x, int y)
{
    return y * Traits::BOARD_SIZE + x;
}

template<typename Traits>
inline void set_bb_generic(typename Traits::Bitboard &bb, int x, int y)
{
	int idx = index_bb_generic<Traits>(x, y);
	bb[idx / 64] |= (1ULL << (idx % 64));
}

template<typename Traits>
inline bool get_bb_generic(const typename Traits::Bitboard &bb, int x, int y)
{
	int idx = index_bb_generic<Traits>(x, y);
	return (bb[idx / 64] & (1ULL << (idx % 64))) != 0;
}

template<typename Traits>
inline void clear_bit_generic(typename Traits::Bitboard &bb, int x, int y)
{
	int idx = index_bb_generic<Traits>(x, y);
	bb[idx >> 6] &= ~(1ULL << (idx & 63));
}


template<typename Traits>
inline bool in_board_generic(int x, int y)
{
	return x >= 0 && x < Traits::BOARD_SIZE && y >= 0 && y < Traits::BOARD_SIZE;
}

template<typename Traits>
inline int popcount_bb_generic(const typename Traits::Bitboard& bb)
{
	int n = 0;

	for (int i = 0; i < Traits::WORD_COUNT; i++)
		n += __builtin_popcountll(bb[i]);
	return n;
}

// inline void clear_bit(bitboard19 &bb, int x, int y)
// {
//     int idx = index_bb19(x, y);
//     bb[idx >> 6] &= ~(1ULL << (idx & 63));
// }

// inline bool in_board(int x, int y)
// {
// 	return x >= 0 && x < 19 && y >= 0 && y < 19;
// }

// // counts set bits across all 6 words (uint64_t)
// inline int popcount_bb(const bitboard19& bb)
// {
//     int n = 0;
//     for (int i = 0; i < 6; i++) n += __builtin_popcountll(bb[i]);
//     return n;
// }


template<typename Traits>
void print_bb_19(t_BWBoard<Traits> &bw)
{
	std::string str;

	str += "\n - - - - - - - - - - - - - - - - - - -\n";
	for (uint64_t y = 0; y < Traits::BOARD_SIZE; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < Traits::BOARD_SIZE; x++)
		{
			if (get_bb_generic<Traits>(bw.black, x, y) || get_bb_generic<Traits>(bw.white, x, y))
			{
				if (get_bb_generic<Traits>(bw.black, x, y))
					str += "B ";
				else if (get_bb_generic<Traits>(bw.white, x, y))
					str += "W ";
			}	
			else
				str += "0 ";
		}
		str += "|\n";
	}
	str += " - - - - - - - - - - - - - - - - - - -\n";
	std::cout << str << std::endl;
}

template<typename Traits>
void print_bb_19_colored(t_BWBoard<Traits> &bw)
{
	std::string str;

	str += "\n - - - - - - - - - - - - - - - - - - -\n";
	for (uint64_t y = 0; y < Traits::BOARD_SIZE; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < Traits::BOARD_SIZE; x++)
		{
			uint64_t idx = index_bb_generic<Traits>(x, y);

			if (get_bb_generic<Traits>(bw.black, x, y))
				str += BLACK_STONE "B " RESET;
			else if (get_bb_generic<Traits>(bw.white, x, y))
				str += WHITE_STONE "W " RESET;
			else
				str += EMPTY_CELL ". " RESET;
		}
		str += "|\n";
	}
	str += " - - - - - - - - - - - - - - - - - - -\n";

	std::cout << str << std::endl;
}

template<typename Traits>
t_BWBoard<Traits> GameBoard_to_bitboard(const GameBoard &board)
{
	t_BWBoard<Traits> bw_board = {};

	for (int y = 0; y < board.getSize(); y++)
	{
		for (int x = 0; x < board.getSize(); x++)
		{
			auto cell = board.getCell(x, y);

			if (cell == CellStatus::Black)
				set_bb_generic<Traits>(bw_board.black, x, y);
			else if (cell == CellStatus::White)
				set_bb_generic<Traits>(bw_board.white, x, y);
		}
	}
	return bw_board;
}

template<typename Traits>
bool detect_captures(const t_BWBoard<Traits>& board, int col, int row, Color attackerColor, typename Traits::Bitboard& capturedMask)
{
	const typename Traits::Bitboard& attacker = (attackerColor == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& victime = (attackerColor == Color::Black) ? board.white : board.black;

	bool captured = false;

	for (Direction dir : LINE_DIRS)
	{
		for (int sign : {-1, 1})
		{
			int stepX = sign * dx(dir);
			int stepY = sign * dy(dir);

			int x1 = col + 1 * stepX, y1 = row + 1 * stepY;
			int x2 = col + 2 * stepX, y2 = row + 2 * stepY;
			int x3 = col + 3 * stepX, y3 = row + 3 * stepY;

			if (!in_board_generic<Traits>(x1, y1) || !in_board_generic<Traits>(x2, y2) || !in_board_generic<Traits>(x3, y3))
				continue;

			int innerVictim = index_bb_generic<Traits>(x1, y1);
			int outerVictim = index_bb_generic<Traits>(x2, y2);
			int flank       = index_bb_generic<Traits>(x3, y3);

			if (get_bb_generic<Traits>(victime, innerVictim) && get_bb_generic<Traits>(victime, outerVictim) && get_bb_generic<Traits>(attacker, flank))
			{
				capturedMask[innerVictim >> 6] |= (1ULL << (innerVictim & 63));
				capturedMask[outerVictim >> 6] |= (1ULL << (outerVictim & 63));
				captured = true;
			}
		}
	}

	return captured;
}

template<typename Traits>
void apply_captures(t_BWBoard<Traits>& board, const typename Traits::Bitboard captured, Color attacker)
{
	typename Traits::Bitboard& victimBitboard = (attacker == Color::Black) ? board.white : board.black;
	for (int i = 0; i < Traits::WORD_COUNT; i++)
		victimBitboard[i] &= ~captured[i];
}


// void	print_bb_19(t_BWBoard19 &bb);
// void	print_bb_19_colored(t_BWBoard19 &bw);
void	print_binaire64(const uint64_t ut);
void	print_binaire_board19(const bitboard19 bb);
			
// t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board);

// Writes into `capturedMask` the bitmask of opponent stones flanked by placing `attacker` at (col, row).
// All four directions are checked. `capturedMask` must be zeroed by the caller.
// Returns true if at least one capture was found.
// bool detect_captures(const t_BWBoard19& board, int col, int row, Color attacker, bitboard19& capturedMask);

// Removes the bits in `captured` from the victim's color board.
// void apply_captures(t_BWBoard19& board, const bitboard19 captured, Color attacker);


#endif // BITBOARD_HPP