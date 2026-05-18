#include "bitboard/bitboard.hpp"

// testing

// inline int index(int x, int y) {
// 	return y * 20 + x;
// }

// inline void set(bitboard19 &bb, int x, int y)
// {
// 	int idx = index(x, y);
// 	bb[idx / 64] |= (1ULL << (idx % 64));
// }

// inline bool get(const bitboard19& bb, int i) {
// 	return (bb[i >> 6] >> (i & 63)) & 1ULL;
// }

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

// #define BLACK_STONE "\033[1;30m"   // bright black (gray)
// #define WHITE_STONE "\033[1;37m"   // bright white
// #define EMPTY_CELL  "\033[2;37m"   // dim white

// void print_bb_19_colored(t_BWBoard19 &bw)
// {
// 	std::string str;

// 	str += "\n - - - - - - - - - - - - - - - - - - -\n";
// 	for (uint64_t y = 0; y < 19; y++)
// 	{
// 		str += '|';
// 		for (uint64_t x = 0; x < 19; x++)
// 		{
// 			uint64_t idx = index_bb19(x, y);

// 			if (get_bb19(bw.black, idx))
// 				str += BLACK_STONE "B " RESET;
// 			else if (get_bb19(bw.white, idx))
// 				str += WHITE_STONE "W " RESET;
// 			else
// 				str += EMPTY_CELL ". " RESET;
// 		}
// 		str += "|\n";
// 	}
// 	str += " - - - - - - - - - - - - - - - - - - -\n";

// 	std::cout << str << std::endl;
// }



// void print_bb_19(t_BWBoard19 &bw)
// {
// 	std::string str;

// 	str += "\n - - - - - - - - - - - - - - - - - - -\n";
// 	for (uint64_t y = 0; y < 19; y++)
// 	{
// 		str += '|';
// 		for (uint64_t x = 0; x < 19; x++)
// 		{
// 			if (get_bb19(bw.black, index_bb19(x, y)) || get_bb19(bw.white, index_bb19(x, y)))
// 			{
// 				if (get_bb19(bw.black, index_bb19(x, y)))
// 					str += "B ";
// 				else if (get_bb19(bw.white, index_bb19(x, y)))
// 					str += "W ";
// 			}	
// 			else
// 				str += "0 ";
// 		}
// 		str += "|\n";
// 	}
// 	str += " - - - - - - - - - - - - - - - - - - -\n";
// 	std::cout << str << std::endl;
// }


// t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board)
// {
// 	t_BWBoard19 bw_board = {};

// 	for (int y = 0; y < board.getSize(); y++)
// 	{
// 		for (int x = 0; x < board.getSize(); x++)
// 		{
// 			auto cell = board.getCell(x, y);

// 			if (cell == CellStatus::Black)
// 				set_bb19(bw_board.black, x, y);
// 			else if (cell == CellStatus::White)
// 				set_bb19(bw_board.white, x, y);
// 		}
// 	}
// 	return bw_board;
// }


void	print_binaire64(const uint64_t ut)
{
	for (size_t i = 0; i < 64; i++)
	{	
		if (i % 8 == 0)
			std::cout << ' ';
		if (ut & (1ULL << i))
			std::cout << 1;
		else
			std::cout << 0;
	}
	std::cout << std::endl;
}

void	print_binaire_board19(const bitboard19 bb)
{
	for (size_t i = 0; i < 6; i++)
		print_binaire64(bb[i]);
}



// ── capture detection ─────────────────────────────────────────────────────────

// bool detect_captures(const t_BWBoard19& board, int col, int row, Color attackerColor, bitboard19& capturedMask)
// {
// 	const bitboard19& attacker = (attackerColor == Color::Black) ? board.black : board.white;
// 	const bitboard19& victime = (attackerColor == Color::Black) ? board.white : board.black;

// 	bool captured = false;

// 	for (Direction dir : LINE_DIRS)
// 	{
// 		for (int sign : {-1, 1})
// 		{
// 			int stepX = sign * dx(dir);
// 			int stepY = sign * dy(dir);

// 			int x1 = col + 1 * stepX, y1 = row + 1 * stepY;
// 			int x2 = col + 2 * stepX, y2 = row + 2 * stepY;
// 			int x3 = col + 3 * stepX, y3 = row + 3 * stepY;

// 			if (!in_board(x1, y1) || !in_board(x2, y2) || !in_board(x3, y3))
// 				continue;

// 			int innerVictim = index_bb19(x1, y1);
// 			int outerVictim = index_bb19(x2, y2);
// 			int flank       = index_bb19(x3, y3);

// 			if (get_bb19(victime, innerVictim) && get_bb19(victime, outerVictim) && get_bb19(attacker, flank))
// 			{
// 				capturedMask[innerVictim >> 6] |= (1ULL << (innerVictim & 63));
// 				capturedMask[outerVictim >> 6] |= (1ULL << (outerVictim & 63));
// 				captured = true;
// 			}
// 		}
// 	}

// 	return captured;
// }

// void apply_captures(t_BWBoard19& board, const bitboard19 captured, Color attacker)
// {
// 	bitboard19& victimBitboard = (attacker == Color::Black) ? board.white : board.black;
// 	for (int i = 0; i < 6; i++)
// 		victimBitboard[i] &= ~captured[i];
// }
