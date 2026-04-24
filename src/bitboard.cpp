#include "../include/bitboard.hpp"

// testing



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


// Version ultra rapide avec table de lookup, la plus efficace a ce jour



// Stockage global de tous les masks (pour référence)
bitboard19 all_masks5[MAX_WINNING_MASK];
int total_masks5 = 0;



// Table 1D uniquement
MaskList lookup_table5[361][4];  // 19*19 = 361

// Construction modifiée
void add_mask_to_lookup(int mask_index, int start_pos, int stride)
{
	int dir;
	
	if (stride == 1)
		dir = DIR_HORIZ;
	else if (stride == 20)
		dir = DIR_VERT;
	else if (stride == 21)
		dir = DIR_DIAG_G;
	else if (stride == 19)
		dir = DIR_DIAG_D;
	else
		return;
	
	for (int i = 0; i < 5; i++) {
		int pos = start_pos + i * stride;
		int y = pos / 20;
		int x = pos % 20;
		int idx = IDX(x, y);  // Calcul une fois
		
		MaskList *list = &lookup_table5[idx][dir];
		
		for (int w = 0; w < 6; w++) {
			list->masks[list->count][w] = all_masks5[mask_index][w];
		}
		list->count++;
		if (start_pos == 0 || start_pos == 10)
		{	std::cout << "verif in add mask lookup count = " << list->count << std::endl; 
			std::cout <<  "mask index  " << mask_index << std::endl;
			std::cout << "idx = " << idx << "x = " << x << " y = " << y << std::endl;
		}
	}
}

void build_lookup_table5(void) {
	// Initialise la table
	memset(lookup_table5, 0, sizeof(lookup_table5));
	total_masks5 = 0;
	
	// 1. Horizontal (stride = 1)
	for (int y = 0; y < 19; y++) {
		for (int x = 0; x <= 14; x++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			// Stocke le mask
			for (int w = 0; w < 6; w++) {
				all_masks5[total_masks5][w] = mask[w];
			}
			add_mask_to_lookup(total_masks5, start_pos, 1);
			total_masks5++;
		}
	}
	
	// 2. Vertical (stride = 20)
	for (int x = 0; x < 19; x++) {
		for (int y = 0; y <= 14; y++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i * 20;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			for (int w = 0; w < 6; w++) {
				all_masks5[total_masks5][w] = mask[w];
			}
			add_mask_to_lookup(total_masks5, start_pos, 20);
			total_masks5++;
		}
	}
	
	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y <= 14; y++) {
		for (int x = 0; x <= 14; x++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i * 21;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			for (int w = 0; w < 6; w++) {
				all_masks5[total_masks5][w] = mask[w];
			}
			add_mask_to_lookup(total_masks5, start_pos, 21);
			total_masks5++;
		}
	}
	
	// 4. Diagonale / (stride = 19)
	for (int y = 0; y <= 14; y++) {
		for (int x = 4; x < 19; x++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i * 19;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			for (int w = 0; w < 6; w++) {
				all_masks5[total_masks5][w] = mask[w];
			}
			add_mask_to_lookup(total_masks5, start_pos, 19);
			total_masks5++;
		}
	}
	// std::cout << "total mask " << total_masks5 << std::endl;
}


// Vérification ultra rapide
static inline int isWin_ultra(const bitboard19 bboard, const int x, const int y)
{
	const int idx = IDX(x, y);  // Précalculé

	t_BWBoard19 board = {};
	
	for (int dir = 0; dir < 4; dir++) {
		MaskList *list = &lookup_table5[idx][dir];
		int count = list->count;
		std::cout << "test ultra: idx = " << idx << " count " << count << " dir = " << dir << std::endl;
		

		for (int i = 0; i < count; i++) {
			uint64_t (*mask)[6] = &list->masks[i];
			memcpy(board.black, list->masks[i], 6 * sizeof(uint64_t));
			if ((((*mask)[0] & bboard[0]) != (*mask)[0]) ||
			(((*mask)[1] & bboard[1]) != (*mask)[1]) ||
			(((*mask)[2] & bboard[2]) != (*mask)[2]) ||
			(((*mask)[3] & bboard[3]) != (*mask)[3]) ||
			(((*mask)[4] & bboard[4]) != (*mask)[4]) ||
			(((*mask)[5] & bboard[5]) != (*mask)[5]))
			{
				print_bb_19(board);
				continue;
			}
			return 1;
		}
	}
	return 0;
}


bitboard19 all_masks4[MAX_WINNING_MASK];
int total_masks4 = 0;

MaskList4 lookup_table4[361][4]; 


void add_mask_to_lookup4(const int mask_index, const int start_pos, const int stride, const  int left_pos, const int right_pos)
{
	int dir;

	if (stride == 1)
		dir = DIR_HORIZ;
	else if (stride == 20)
		dir = DIR_VERT;
	else if (stride == 21)
		dir = DIR_DIAG_G;
	else if (stride == 19)
		dir = DIR_DIAG_D;
	else return;
	
	for (int i = 0; i < 4; i++) {
		int pos = start_pos + i * stride;
		int y = pos / 20;
		int x = pos % 20;
		int idx = IDX(x, y);  // Calcul une fois
		
		MaskList4 *list = &lookup_table4[idx][dir];
		
		for (int w = 0; w < 6; w++) {
			list->masks[list->count][w] = all_masks4[mask_index][w];
		}
		list->left_pos[list->count] = left_pos;
		list->right_pos[list->count] = right_pos;
		list->count++;
	}
}


void	build_lookup_table4(void) {
	// Initialise la table
	memset(lookup_table4, 0, sizeof(lookup_table4));
	total_masks4 = 0;
	
	// 1. Horizontal (stride = 1)
	for (int y = 0; y < 19; y++) {
		for (int x = 0; x <= 15; x++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 4; i++) {
				int bit = start_pos + i;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			int left_pos = (x > 0) ? (start_pos - 1) : -1;
			int right_pos = (x + 4 < 19) ? (start_pos + 4) : -1;
			// Stocke le mask
			for (int w = 0; w < 6; w++) {
				all_masks4[total_masks4][w] = mask[w];
			}
			add_mask_to_lookup4(total_masks4, start_pos, 1, left_pos, right_pos);
			total_masks4++;
		}
	}
	
	// 2. Vertical (stride = 20)
	for (int x = 0; x < 19; x++) {
		for (int y = 0; y <= 15; y++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 4; i++) {
				int bit = start_pos + i * 20;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			int left_pos = (y > 0) ? (start_pos - 20) : -1;
			int right_pos = (y + 4 < 19) ? (start_pos + 80) : -1;
			for (int w = 0; w < 6; w++) {
				all_masks4[total_masks4][w] = mask[w];
			}
			add_mask_to_lookup4(total_masks4, start_pos, 20, left_pos, right_pos);
			total_masks4++;
		}
	}
	
	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y <= 15; y++) {
		for (int x = 0; x <= 15; x++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 4; i++) {
				int bit = start_pos + i * 21;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			int left_pos = (y > 0 && x > 0) ? (start_pos - 21) : -1;
			int right_pos = (y + 4 < 19 && x + 4 < 19) ? (start_pos + 84) : -1;
			for (int w = 0; w < 6; w++) {
				all_masks4[total_masks4][w] = mask[w];
			}
			add_mask_to_lookup4(total_masks4, start_pos, 21, left_pos, right_pos);
			total_masks4++;
		}
	}
	
	// 4. Diagonale / (stride = 19)
	for (int y = 0; y <= 15; y++) {
		for (int x = 3; x < 19; x++) {
			bitboard19 mask = {0};
			int start_pos = y * 20 + x;
			for (int i = 0; i < 4; i++) {
				int bit = start_pos + i * 19;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			int left_pos = (y > 0 && x + 4 < 19) ? (start_pos - 19) : -1;
			int right_pos = (y + 4 < 19 && x > 0) ? (start_pos + 76) : -1;
			for (int w = 0; w < 6; w++) {
				all_masks4[total_masks4][w] = mask[w];
			}
			add_mask_to_lookup4(total_masks4, start_pos, 19, left_pos, right_pos);
			total_masks4++;
		}
	}
}

static inline int is_Open_4(const bitboard19 &boardA, const bitboard19 &boardB, const int x, const int y) {
	const int idx = IDX(x, y);  // Précalculé
	int	opening_score = 2;

	// si l'opening score est a zero, il n'y a pas d'alignement de 4 pierre
	// ou il y a un alignement mais il est completement ferme.
	// si l'opening score est a 1, c'est que l'alignement de 4 pierre est partiellement ouvert.
	// si le score est  a 2, c'est que c'est un open 4.

	for (int dir = 0; dir < 4; dir++) {
		MaskList4 *list = &lookup_table4[idx][dir];
		int count = list->count;
		
		for (int i = 0; i < count; i++) {
			uint64_t (*mask)[6] = &list->masks[i];
			
			if ((((*mask)[0] & boardA[0]) != (*mask)[0]) ||
				(((*mask)[1] & boardA[1]) != (*mask)[1]) ||
				(((*mask)[2] & boardA[2]) != (*mask)[2]) ||
				(((*mask)[3] & boardA[3]) != (*mask)[3]) ||
				(((*mask)[4] & boardA[4]) != (*mask)[4]) ||
				(((*mask)[5] & boardA[5]) != (*mask)[5]))
			{
				continue;
			}
			if (list->left_pos[i] == -1 || get(boardB, list->left_pos[i]))
				opening_score -= 1; 
			if (list->right_pos[i] == -1 || get(boardB, list->right_pos[i]))
				opening_score -= 1;
			return opening_score;
		}
	}
	return 0;
}

void	test_bitboard(const GameBoard& board, int x, int y)
{
	t_BWBoard19 bitboard = GameBoard_to_bitboard(board);

	print_bb_19(bitboard);

	// bitboard19	winning_mask[MAX_WINNING_MASK] = {};

	// test_pattern(winning_mask);
	// if (isWin(bitboard.black, winning_mask))
	// 	std::cout << "les noires ont gagne !!!" << std::endl;
	// if (isWin(bitboard.white, winning_mask))
	// 	std::cout << "les blanches ont gagne !!!" << std::endl;
	std::cout << "test bitboard : x = " << x << " y = " << y << std::endl;
	build_lookup_table5();
	build_lookup_table4();
	if (isWin_ultra(bitboard.black, x, y))
		std::cout << "les noires ont gagne !!!" << std::endl;
	if (isWin_ultra(bitboard.white, x, y))
		std::cout << "les blanches ont gagne !!!" << std::endl;

	int	black_four = is_Open_4(bitboard.black, bitboard.white, x, y);
	int white_four = is_Open_4(bitboard.white, bitboard.black, x, y);
	if (black_four == 1)
		std::cout << "alignement de 4 pierre noir partiellement ouvert" << std::endl;
	else if (black_four == 2)
		std::cout << "Open four black !!!" << std::endl;
	if (white_four == 1)
		std::cout << "alignement de 4 pierre blanche partiellement ouvert" << std::endl;
	else if (white_four == 2)
		std::cout << "Open four white !!!" << std::endl;
	
	
}
