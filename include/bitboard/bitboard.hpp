#ifndef BITBOARD_HPP
# define BITBOARD_HPP

#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "direction.hpp"

#include <cstdint>
#include <iostream>
#include <array>

#define BLACK_STONE "\033[1;30m"   // bright black (gray)
#define WHITE_STONE "\033[1;37m"   // bright white
#define LEGAL_MOVE  "\033[1;32m"   // bright green
#define EMPTY_CELL  "\033[2;37m"   // dim white
#define TITLE_LINE "\033[1;34m"   // bright blue
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


template<typename Traits>
inline constexpr std::size_t MAX_BOARD_MOVES = Traits::BOARD_SIZE * Traits::BOARD_SIZE - (Traits::BOARD_SIZE * Traits::BOARD_SIZE) / 7;

template<typename Traits>
struct t_BWBoard
{
    typename Traits::Bitboard black;
    typename Traits::Bitboard white;
};

typedef t_BWBoard<BoardTraits<19>>     t_BWBoard19;
typedef t_BWBoard<BoardTraits<15>>     t_BWBoard15;

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

// Read a bit by its flat physical index (y*STRIDE+x).
// Used by pattern matching code that stores pre-computed flat indices.
// Returns false for the -1 sentinel used by pattern structs.
template<typename Traits>
inline bool get_bb_flate(const typename Traits::Bitboard& bb, int idx)
{
	return (bb[idx / 64] & (1ULL << (idx % 64))) != 0;
}


template<typename Traits>
inline void clear_bit_generic(typename Traits::Bitboard &bb, int x, int y)
{
	int idx = index_bb_generic<Traits>(x, y);
	bb[idx >> 6] &= ~(1ULL << (idx & 63));
}

template<typename Traits>
inline typename Traits::Bitboard& bitboardForColor(t_BWBoard<Traits>& board, const Color color)
{
	return (color == Color::Black) ? board.black : board.white;
}

template<typename Traits>
inline const typename Traits::Bitboard& bitboardForColor(const t_BWBoard<Traits>& board, const Color color)
{
	return (color == Color::Black) ? board.black : board.white;
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

// Calls fn(x, y) for each set bit in bb, skipping stride-padding positions.
template<typename Traits, typename Fn>
inline void bb_for_each_bit(const typename Traits::Bitboard& bb, Fn fn)
{
    for (int w = 0; w < Traits::WORD_COUNT; w++)
    {
        uint64_t word = bb[w];
        while (word)
        {
            int bit = __builtin_ctzll(word);
            word &= word - 1;
            int pos = w * 64 + bit;
            int x   = pos % Traits::STRIDE;
            int y   = pos / Traits::STRIDE;
            if (x < Traits::BOARD_SIZE && y < Traits::BOARD_SIZE)
                fn(x, y);
        }
    }
}

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
					str += BLACK_STONE "B" RESET " ";
				else if (get_bb_generic<Traits>(bw.white, x, y))
					str += WHITE_STONE "W" RESET " ";
			}	
			else
				str += EMPTY_CELL "." RESET " ";
		}
		str += "|\n";
	}
	str += " - - - - - - - - - - - - - - - - - - -\n";
	std::cout << str << std::endl;
}

template<typename Traits>
void print_bb_colored(typename Traits::Bitboard& bb)
{
	std::string str;

	str += "\n";
	for (int i = 0; i < Traits::BOARD_SIZE + 1; i++) {
		str += "--";
	}
	str += "\n";

	for (uint64_t y = 0; y < Traits::BOARD_SIZE; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < Traits::BOARD_SIZE; x++)
		{
			if (get_bb_generic<Traits>(bb, x, y))
				str += LEGAL_MOVE "* " RESET;
			else
				str += EMPTY_CELL ". " RESET;
		}
		str += "|\n";
	}

	for (int i = 0; i < Traits::BOARD_SIZE + 1; i++) {
		str += "--";
	}
	str += "\n";

	std::cout << str << std::endl;
}

template<typename Traits>
void print_bb_overlay(t_BWBoard<Traits>& bw, typename Traits::Bitboard& legalMoves)
{
	std::string str;

	str += "\n";
	for (int i = 0; i < Traits::BOARD_SIZE + 1; i++)
		str += "--";
	str += "\n";

	for (uint64_t y = 0; y < Traits::BOARD_SIZE; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < Traits::BOARD_SIZE; x++)
		{
			if (get_bb_generic<Traits>(bw.black, x, y))
				str += BLACK_STONE "B " RESET;
			else if (get_bb_generic<Traits>(bw.white, x, y))
				str += WHITE_STONE "W " RESET;
			else if (get_bb_generic<Traits>(legalMoves, x, y))
				str += LEGAL_MOVE "* " RESET;
			else
				str += EMPTY_CELL ". " RESET;
		}
		str += "|\n";
	}

	for (int i = 0; i < Traits::BOARD_SIZE + 1; i++)
		str += "--";
	str += "\n";

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
bool detect_captures(const t_BWBoard<Traits>& board, int col, int row, const Color attackerColor, typename Traits::Bitboard& capturedMask)
{
	const typename Traits::Bitboard& attacker = bitboardForColor(board, attackerColor);
	const Color victimColor = (attackerColor == Color::Black) ? Color::White : Color::Black;
	const typename Traits::Bitboard& victime = bitboardForColor(board, victimColor);

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

			if (get_bb_generic<Traits>(victime, x1, y1) &&
				get_bb_generic<Traits>(victime, x2, y2) &&
				get_bb_generic<Traits>(attacker, x3, y3))
			{
				set_bb_generic<Traits>(capturedMask, x1, y1);
				set_bb_generic<Traits>(capturedMask, x2, y2);
				captured = true;
			}
		}
	}

	return captured;
}


template<typename Traits>
void apply_captures(t_BWBoard<Traits>& board, const typename Traits::Bitboard captured, const Color attacker)
{
	const Color victimColor = (attacker == Color::Black) ? Color::White : Color::Black;
	typename Traits::Bitboard& victimBitboard = bitboardForColor(board, victimColor);
	for (int i = 0; i < Traits::WORD_COUNT; i++)
		victimBitboard[i] &= ~captured[i];
}

#endif // BITBOARD_HPP