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


void	print_binaire64(const uint64_t ut)
{
	size_t bit = 63;

	for (size_t i = 0; i < 64; i++)
	{	
		bit	= 63 - i;
		if (i % 8 == 0)
			std::cout << ' ';
		if (ut & (1ULL << bit))
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
				// std::cout << "line numeros xxx " << i + x << std::endl;
				print_binaire64(tab_int[i + x]);
			}
			i += x;
		}
		else
		{
			tab_int[i] = filtre5 >> i;
			// std::cout << "line numeros " << i << std::endl;
			print_binaire64(tab_int[i]);
		}
		
	}	

	std::cout << "mask line " << std::endl;
	print_binaire64(mask_line);
	return true;
}


// creer un pattern horizontal a la position voulu, on creer un mask global 
void	patern_creation(bitboard19 *bb, int pos, uint64_t pattern)
{
	// ne gere pas les depassement de lignes. le patterne depasse sur l'autre ligne.
	// la fonction appelante doit s'ocuper de verifier si la position est correcte.
	int line = pos / 20;
	pos += line;

	int idx = pos / 64;
	int	offset = pos % 64;

	bb[0][idx] |= pattern >> offset;
	
	if (offset >= 60)
	{
		bb[0][idx + 1] |= pattern << (64 - offset); 
	}
}


// creer des filtre pour tester la condition de victoire.
// prototype, on peut utiliser ce systeme pour des 4 ou des 3 a la suite.
// toujour en horizontal pour le moment
void	test_pattern(void)
{
	uint64_t	raw5 = 0XF800000000000000;
	int	patern_size = 5;

	bitboard19 bb = {};

	for (int i = 0; i < 100; i++)
	{
		std::memset(bb, 0, sizeof(bb));
		if ((i % 20) + patern_size < 20)
		{
			patern_creation(&bb, i, raw5);
			// std::cout << "pos = " << i << std::endl; 
			print_binaire_board19(bb);
			std::cout << std::endl;
		}
	}
}


void	test_bitboard(const GameBoard& board)
{
	t_BWBoard19 bitboard = GameBoard_to_bitboard(board);

	print_bb_19(bitboard);
	// five_detection();
	test_pattern();

}
