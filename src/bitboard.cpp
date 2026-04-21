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
					str += "B ";
				else if (get(bw.white, index(x, y)))
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
				set(bw_board.black, x, y);
			else if (cell == CellStatus::White)
				set(bw_board.white, x, y);
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

bool	five_detection(void)
{
	// les 5 premiers bit de poids fort initialiser a 1
	// __uint64_t filtre = (31ULL << 59);
	__uint64_t filtre5 = 0XF800000000000000;
	__uint64_t filtre6 = 0XFC00000000000000;
	__uint64_t	mask_line = 0XFFFFEFFFFEFFFFEF;

	u_int64_t tab_int[64 - 5];

	for (size_t i = 0; i < 64 -5; i++)
	{
		if ((i + 5) % 20 == 0)
		{
			size_t x = 0;
			for (x = 0; x < 5; x++)
			{
				tab_int[i + x] = (filtre6 >> (i + x)) & mask_line;
				print_binaire64(tab_int[i + x]);
			}
			i += x;
		}
		else
		{
			tab_int[i] = filtre5 >> i;
			print_binaire64(tab_int[i]);
		}
	}	

	std::cout << "mask line " << std::endl;
	print_binaire64(mask_line);
	return true;
}


// creer un pattern horizontal a la position voulu, on creer un mask global 
void	pattern_creation(bitboard19 bb, uint64_t pattern, int size, int pos)
{
	// ne gere pas les depassement de lignes. le patterne depasse sur l'autre ligne.
	// la fonction appelante doit s'ocuper de verifier si la position est correcte.
	int line = pos / 20;
	pos += line;

	int idx = pos / 64;
	int	offset = pos % 64;

	bb[idx] |= pattern << offset;
	
	if (offset > (64 - size))
	{
		bb[idx + 1] |= pattern >> (64 - offset); 
	}
}


void	pattern_universel(bitboard19 bb, int size, int pos, int strides)
{
	int	y = pos / 19;
	int	x = pos % 19;

	int	start = y * 20 + x;
	
	for (int i = 0; i < size; i++)
	{
		int bit = start + i * strides;

		int idx = bit / 64;
		int	offset = bit % 64;

		bb[idx] |= (1ULL << offset);
	}
}




// creer des filtre pour tester la condition de victoire.
// prototype, on peut utiliser ce systeme pour des 4 ou des 3 a la suite.
// toujour en horizontal pour le moment
const int	MAX_WINNING_MASK = 1020;

void	test_pattern(bitboard19	winning_mask[MAX_WINNING_MASK])
{
	// uint64_t	raw5 = 0XF800000000000000;
	// uint64_t	raw5 = 0b11111;
	// bitboard19	bb = {};
	// bitboard19	bw = {};
	// t_BWBoard19 board = {};
	int	pattern_height = 1;
	int	pattern_lenght = 5;
	int total_mask = 0;

	// on commence par les detection horizontal
	for (int i = 0; i <= 361 - pattern_lenght; i++)
	{
		// std::memset(bb, 0, sizeof(bb));
		if ((i % 19) + pattern_lenght < 20)
		{
			bitboard19 bb = {};
			// std::memset(bb, 0, sizeof(bb));

			// pattern_creation(bb, raw5, pattern_size, i);
			pattern_universel(bb, 5, i, 1); // pattern horizontal;
			// std::memcpy(&winning_mask[total_mask * 6], bb, sizeof(bb));
			for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
			total_mask++;
			// print_binaire_board19(bb);
			// print_bb_19(board);
			// std::cout << std::endl;
		}
	}

	// les patternes verticales
	pattern_height = 5;
	for (int i = 0; i < 361 - (pattern_height - 1) * 19; i++)
	{
		bitboard19 bb = {};
		pattern_universel(bb, 5, i, 20);
		for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
		total_mask++;
	}

	// patternes diagonale droite
	for (int i = 0; i < 361 - (pattern_height - 1) * 19; i++)
	{
		if ((i % 19) + pattern_lenght < 20)
		{
			bitboard19 bb = {};
			pattern_universel(bb, 5, i, 21);
			for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
			total_mask++;
		}
	}

	// patternes diagonale gauche
	for (int i = 0; i < 361 - (pattern_height - 1) * 19; i++)
	{
		if ((i % 19) + pattern_lenght < 20)
		{
			bitboard19 bb = {};
			pattern_universel(bb, 5, i + 4, 19);
			for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
			total_mask++;
		}
	}

	// std::cout << "Total mask = " << total_mask << std::endl;
	// for (int i = 0; i < MAX_WINNING_MASK; i++)
	// {
	// 	for (int x = 0; x < 6; x++)
	// 		board.black[x] = winning_mask[i][x];
	// 	print_bb_19(board);
	// 	std::cout << std::endl;
	// }
}


bool	isWin(const bitboard19 bboard, const bitboard19 winning_mask[MAX_WINNING_MASK])
{
	for (int i = 0 ; i < MAX_WINNING_MASK; i++)
	{
		bool win = true;
		for (int x = 0; x < 6; x++)
		{
			if ((winning_mask[i][x] & bboard[x]) != winning_mask[i][x])
			{
				win = false;
				break;
			}
		}
		if (win)
			return (win);
	}
	return (false);
}

void	test_bitboard(const GameBoard& board)
{
	t_BWBoard19 bitboard = GameBoard_to_bitboard(board);

	print_bb_19(bitboard);
	// std::cout << "bitboard black" << std::endl;
	// print_binaire_board19(bitboard.black);
	// std::cout << "\nbitboard white" << std::endl;
	// print_binaire_board19(bitboard.white);
	// std::cout << std::endl;

	// five_detection();
	bitboard19	winning_mask[MAX_WINNING_MASK] = {};

	test_pattern(winning_mask);
	if (isWin(bitboard.black, winning_mask))
		std::cout << "les noires ont gagne !!!" << std::endl;
	if (isWin(bitboard.white, winning_mask))
		std::cout << "les blanches ont gagne !!!" << std::endl;

}
