#include "../include/bitboard/bitboard.hpp"

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


void print_bb_19(t_BWBoard19 &bw)
{
	std::string str;

	str += "\n - - - - - - - - - - - - - - - - - - -\n";
	for (uint64_t y = 0; y < 19; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < 19; x++)
		{
			if (get_bb19(bw.black, index_bb19(x, y)) || get_bb19(bw.white, index_bb19(x, y)))
			{
				if (get_bb19(bw.black, index_bb19(x, y)))
					str += "B ";
				else if (get_bb19(bw.white, index_bb19(x, y)))
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


t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board)
{
	t_BWBoard19 bw_board = {};

	for (int y = 0; y < board.getSize(); y++)
	{
		for (int x = 0; x < board.getSize(); x++)
		{
			auto cell = board.getCell(x, y);

			if (cell == CellStatus::Black)
				set_bb19(bw_board.black, x, y);
			else if (cell == CellStatus::White)
				set_bb19(bw_board.white, x, y);
		}
	}
	return bw_board;
}


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
