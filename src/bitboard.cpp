#include "../include/bitboard.hpp"

// testing

inline int index(int x, int y) {
	return y * 20 + x;
}


// set le bit a 1 a la position x, y
inline void set(bitboard19 &bb, int x, int y)
{
	int idx = index(x, y);
	bb[idx / 64] |= (1ULL << (idx % 64));
}

inline bool get(const bitboard19& bb, int i) {
	return (bb[i >> 6] >> (i & 63)) & 1ULL;
}


void print_bb_19(t_BWBoard19 &bw)
{
	std::string str;

	str += "\n - - - - - - - - - - - - - - - - - - -\n";
	for (uint64_t y = 0; y < 19; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < 19; x++)
		{
			if (get(bw.black, index(x, y)) || get(bw.white, index(x, y)))
			{
				if (get(bw.black, index(x, y)))
				{
					str += "B ";
				}
				else if (get(bw.white, index(x, y)))
				{
					str += "W ";
				}
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
				set(bw_board.black, x, y);
			else if (cell == CellStatus::White)
				set(bw_board.white, x, y);
		}
	}
	return bw_board;
}



void	test_bitboard(const GameBoard& board)
{
	t_BWBoard19 bitboard = GameBoard_to_bitboard(board);

	print_bb_19(bitboard);
}