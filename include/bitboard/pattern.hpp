#ifndef PATTERN_HPP
#define PATTERN_HPP

#include "bitboard.hpp"
#include "string.h"


#define DIR_HORIZ 0
#define DIR_VERT  1
#define DIR_DIAG_G 2
#define DIR_DIAG_D 3 

template<typename Traits>
struct t_PatternList5
{
	typename Traits::Bitboard masks[5];  // 5 masks × 6 uint64_t = 240 bytes
	uint32_t count;          // 4 bytes
	uint8_t padding[4]; // 12 bytes de padding pour aligner à 248 bytes
};  // 248 bytes total

//---------------------------------------------------------------------------

template<typename Traits>
struct t_super4{
	typename Traits::Bitboard	mask;
	int		hole_pos[2];
};

template<typename Traits>
struct t_PatternList_super4{
	t_super4<Traits>	patterns[7]; // 7 positions relatives de 4 pierres alignées, soit 7*128 = 896 bytes
	uint32_t	count;
};

//---------------------------------------------------------------------------

template<typename Traits>
struct t_Pattern4{
	typename Traits::Bitboard	mask;
	int			opposant_left;
	int			opposant_right;
};

template<typename Traits>
struct t_PatternList4{
	t_Pattern4<Traits>	patterns[4];
	int			count;
};

//---------------------------------------------------------------------------

template<typename Traits>
struct t_PatternGroup4{
	typename Traits::Bitboard	masks[3];  // 3 masks × 6 uint64_t = 144 bytes
	int			hole_pos[3];	// 12 bytes
}; // 156 bytes total

template <typename Traits>
struct t_PatternList_Groupe4{
	t_PatternGroup4<Traits>	patterns[5];	// 780 bytes
	uint32_t		count;		// 4
	uint8_t			padding[16];
}; 	// 800 bytes total

//---------------------------------------------------------------------------


typedef struct {
	int		stone_pos[4];	// 16 bytes
	int		hole_pos[2];	// 8 bytes
	int		oposant_pos[5];	// 16 bytes
	// int		count;	// 4 bytes
}	t_PatternGroupe3;

typedef struct {
	t_PatternGroupe3 patterns[4]; // 4 positions relatives de 3 pierres alignées, soit 4*44 = 176 bytes
	uint32_t count; // 4 bytes
} t_PatternList_Groupe3;

//---------------------------------------------------------------------------


typedef struct {
	int	up;
	int	down;
	int	left;
	int	right;
	int	middle;
	int	opposant_up[2];
	int	opposant_down[2];
	int	opposant_left[2];
	int	opposant_right[2];
}	t_cross;

typedef struct {
	t_cross	cross[5]; // 5 positions relatives
	int		count;
}	t_PatternList_Cross;

//---------------------------------------------------------------------------

// normalement avec l'optimisation du compilateur, option 02 ou 03,
// la boucle devrait etre deroulee et les 6 uint64_t du mask devraient etre comparés en parallèle,
// ce qui rend la fonction très rapide malgré la taille du mask (384 bytes pour 6 uint64_t).
template<typename Traits>
inline bool match_pattern(
    const typename Traits::Bitboard& pat,
    const typename Traits::Bitboard& board)
{
    for (size_t i = 0; i < pat.size(); i++)
    {
        if ((pat[i] & board[i]) != pat[i])
            return false;
    }

    return true;
}

template<typename Traits>
static void add_mask_to_lookup5(t_PatternList5<Traits> lookup_table5[Traits::CELL_COUNT][4], const typename Traits::Bitboard& mask, const int start_pos, const int stride)
{
	int dir;

	if (stride == 1)
		dir = DIR_HORIZ;
	else if (stride == Traits::STRIDE)
		dir = DIR_VERT;
	else if (stride == Traits::STRIDE_G)
		dir = DIR_DIAG_G;
	else if (stride == Traits::STRIDE_D)
		dir = DIR_DIAG_D;
	else
		return;
	
	for (int i = 0; i < 5; i++)
	{
		int pos = start_pos + i * stride;
		int y = pos / Traits::STRIDE;
		int x = pos % Traits::STRIDE;
		int idx = idx_generic<Traits>(x, y);
		
		t_PatternList5<Traits> *list = &lookup_table5[idx][dir];
		
		for (int w = 0; w < Traits::WORD_COUNT; w++)
			list->masks[list->count][w] = mask[w];
		list->count++;
	}
}



template<typename Traits>
int	isWin_ultra(t_PatternList5<Traits> lookup_table5[Traits::CELL_COUNT][4], const typename Traits::Bitboard& board,
	const int x, const int y)
{
	const int idx = idx_generic<Traits>(x, y);

	for (int dir = 0; dir < 4; dir++)
	{
		t_PatternList5<Traits> *list = &lookup_table5[idx][dir];
		int count = list->count;

		for (int i = 0; i < count; i++)
			if (match_pattern<Traits>(list->masks[i], board))
				return 1;
	}
	return 0;
}

template<typename Traits>
void build_lookup_table5(t_PatternList5<Traits> lookup_table5[Traits::CELL_COUNT][4])
{

	memset(lookup_table5, 0, sizeof(t_PatternList5<Traits>) * Traits::CELL_COUNT * 4);
	// 1. Horizontal (stride = 1)
	for (int y = 0; y < Traits::BOARD_SIZE; y++)
	{
		for (int x = 0; x <= Traits::BOARD_SIZE - 5; x++)
		{
			typename Traits::Bitboard mask = {0};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			add_mask_to_lookup5<Traits>(lookup_table5, mask, start_pos, 1);
		}
	}
	
	// 2. Vertical (stride = 20)
	for (int x = 0; x < Traits::BOARD_SIZE; x++)
	{
		for (int y = 0; y <= Traits::BOARD_SIZE - 5; y++)
		{
			typename Traits::Bitboard mask = {0};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i * Traits::STRIDE;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			add_mask_to_lookup5<Traits>(lookup_table5, mask, start_pos, Traits::STRIDE);
		}
	}
	
	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y <= Traits::BOARD_SIZE - 5; y++)
	{
		for (int x = 0; x <= Traits::BOARD_SIZE - 5; x++)
		{
			typename Traits::Bitboard mask = {0};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i * Traits::STRIDE_D;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			add_mask_to_lookup5<Traits>(lookup_table5, mask, start_pos, Traits::STRIDE_D);
		}
	}
	
	// 4. Diagonale / (stride = 19)
	for (int y = 0; y <= Traits::BOARD_SIZE - 5; y++)
	{
		for (int x = 4; x < Traits::BOARD_SIZE; x++)
		{
			typename Traits::Bitboard mask = {0};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i * Traits::STRIDE_G;
				int idx = bit >> 6;
				int offset = bit & 63;
				mask[idx] |= (1ULL << offset);
			}
			add_mask_to_lookup5<Traits>(lookup_table5, mask, start_pos, Traits::STRIDE_G);
		}
	}
}

template<typename Traits>
void add_mask_to_lookup4(t_PatternList4<Traits> lookup_table4[Traits::CELL_COUNT][4], t_Pattern4<Traits> *pattern ,
	const int start_pos, const int stride)
{
	int dir;

	if (stride == 1)
		dir = DIR_HORIZ;
	else if (stride == Traits::STRIDE)
		dir = DIR_VERT;
	else if (stride == Traits::STRIDE_G)
		dir = DIR_DIAG_G;
	else if (stride == Traits::STRIDE_D)
		dir = DIR_DIAG_D;
	else return;

	for (int i = 0; i < 4; i++)
	{
		int pos = start_pos + i * stride;
		int y = pos / Traits::STRIDE;
		int x = pos % Traits::STRIDE;
		int idx = idx_generic<Traits>(x, y);
		
		t_PatternList4<Traits> *list = &lookup_table4[idx][dir];
		
		list->patterns[list->count] = *pattern;
		list->count++;
	}
}

template<typename Traits>
int	is_Open_4(t_PatternList4<Traits> lookup_table4[Traits::CELL_COUNT][4],
			const typename Traits::Bitboard &boardA, const typename Traits::Bitboard &boardB,
				const int x, const int y)
{
	const int idx = idx_generic<Traits>(x, y);
	int	opening_score = 2;

	// si l'opening score est a zero, il n'y a pas d'alignement de 4 pierre
	// ou il y a un alignement mais il est completement ferme.
	// si l'opening score est a 1, c'est que l'alignement de 4 pierre est partiellement ouvert.
	// si le score est  a 2, c'est que c'est un open 4.

	for (int dir = 0; dir < 4; dir++)
	{
		t_PatternList4<Traits> *list = &lookup_table4[idx][dir];
		int count = list->count;
		
		for (int i = 0; i < count; i++)
		{
			t_Pattern4<Traits> *pattern = &list->patterns[i];
			
			if (!match_pattern<Traits>(pattern->mask, boardA)) // Si le pattern ne match pas, on continue
				continue;
			if (pattern->opposant_left == -1 || get_bb_flate<Traits>(boardB, pattern->opposant_left))
				opening_score -= 1;
			if (pattern->opposant_right == -1 || get_bb_flate<Traits>(boardB, pattern->opposant_right))
				opening_score -= 1;
			return opening_score;
		}
	}
	return 0;
}


template<typename Traits>
void	build_lookup_table4(t_PatternList4<Traits> lookup_table4[Traits::CELL_COUNT][4])
{
	memset(lookup_table4, 0, sizeof(t_PatternList4<Traits>) * Traits::CELL_COUNT * 4);
	// 1. Horizontal (stride = 1)
	for (int y = 0; y < Traits::BOARD_SIZE; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 3; x++)
		{
			t_Pattern4<Traits> pattern = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 4; i++)
			{
				int bit = start_pos + i;
				int idx = bit >> 6;
				int offset = bit & 63;
				pattern.mask[idx] |= (1ULL << offset);
			}
			pattern.opposant_left = (x > 0) ? (start_pos - 1) : -1;
			pattern.opposant_right = (x + 4 < Traits::BOARD_SIZE) ? (start_pos + 4) : -1;

			add_mask_to_lookup4<Traits>(lookup_table4, &pattern, start_pos, 1);
		}
	}
	
	// 2. Vertical (stride = 20)
	for (int x = 0; x < Traits::BOARD_SIZE; x++)
	{
		for (int y = 0; y < Traits::BOARD_SIZE - 3; y++)
		{
			t_Pattern4<Traits> pattern = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 4; i++)
			{
				int bit = start_pos + i * Traits::STRIDE;
				int idx = bit >> 6;
				int offset = bit & 63;
				pattern.mask[idx] |= (1ULL << offset);
			}
			pattern.opposant_left = (y > 0) ? (start_pos - Traits::STRIDE) : -1;
			pattern.opposant_right = (y + 4 < Traits::BOARD_SIZE) ? (start_pos + 4 * Traits::STRIDE) : -1;
			add_mask_to_lookup4<Traits>(lookup_table4, &pattern, start_pos, Traits::STRIDE);
		}
	}
	
	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y < Traits::BOARD_SIZE - 3; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 3; x++)
		{
			t_Pattern4<Traits> pattern = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 4; i++)
			{
				int bit = start_pos + i * Traits::STRIDE_D;
				int idx = bit >> 6;
				int offset = bit & 63;
				pattern.mask[idx] |= (1ULL << offset);
			}
			pattern.opposant_left = (y > 0 && x > 0) ? (start_pos - Traits::STRIDE_D) : -1;
			pattern.opposant_right = (y + 4 < Traits::BOARD_SIZE && x + 4 < Traits::BOARD_SIZE) ? (start_pos + 4 * Traits::STRIDE_D) : -1;
			add_mask_to_lookup4<Traits>(lookup_table4, &pattern, start_pos, Traits::STRIDE_D);
		}
	}
	
	// 4. Diagonale / (stride = 19)
	for (int y = 0; y < Traits::BOARD_SIZE - 3; y++)
	{
		for (int x = 3; x < Traits::BOARD_SIZE; x++)
		{
			t_Pattern4<Traits> pattern = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 4; i++)
			{
				int bit = start_pos + i * Traits::BOARD_SIZE;
				int idx = bit >> 6;
				int offset = bit & 63;
				pattern.mask[idx] |= (1ULL << offset);
			}
			pattern.opposant_left = (y > 0 && x + 4 < Traits::BOARD_SIZE) ? (start_pos - Traits::BOARD_SIZE) : -1;
			pattern.opposant_right = (y + 4 < Traits::BOARD_SIZE && x > 0) ? (start_pos + Traits::BOARD_SIZE * 4) : -1;
			add_mask_to_lookup4<Traits>(lookup_table4, &pattern, start_pos, Traits::BOARD_SIZE);
		}
	}
}


template<typename Traits>
static void add_pattern_group4(t_PatternList_Groupe4<Traits> lookup_table[Traits::CELL_COUNT][4],
                                        t_PatternGroup4<Traits> *group,
                                        int start_x, int start_y, int dir)
{
    for (int i = 0; i < 5; i++)
	{
        int case_x, case_y;
        switch (dir)
		{
            case DIR_HORIZ:
                case_x = start_x + i;
                case_y = start_y;
                break;
            case DIR_VERT:
                case_x = start_x;
                case_y = start_y + i;
                break;
            case DIR_DIAG_G:
                case_x = start_x - i;
                case_y = start_y + i;
                break;
            case DIR_DIAG_D:
                case_x = start_x + i;
                case_y = start_y + i;
                break;
        }
        // Sécurité : on reste dans le plateau
        if (case_x < 0 || case_x >= Traits::BOARD_SIZE || case_y < 0 || case_y >= Traits::BOARD_SIZE)
            continue;

        int cell = idx_generic<Traits>(case_x, case_y);
        t_PatternList_Groupe4<Traits> *list = &lookup_table[cell][dir];

        list->patterns[list->count] = *group;
        list->count++;
    }
}

template<typename Traits>
void build_lookup_table_groupe4(t_PatternList_Groupe4<Traits> lookup_table_groupe4[Traits::CELL_COUNT][4])
{
	memset(lookup_table_groupe4, 0, sizeof(t_PatternList_Groupe4<Traits>) * Traits::CELL_COUNT * 4);
	// 1. Horizontal (stride = 1)
	for (int y = 0; y < Traits::BOARD_SIZE; y++)
	{
		for (int x = 0; x <= Traits::BOARD_SIZE - 5; x++)
		{
			t_PatternGroup4<Traits> gmask = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.masks[0][idx] |= (1ULL << offset);
				gmask.masks[1][idx] |= (1ULL << offset);
				gmask.masks[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit_generic<Traits>(gmask.masks[i], x + 1 + i, y );
				gmask.hole_pos[i] = index_bb_generic<Traits>(x + 1 + i, y);
			}
			add_pattern_group4<Traits>(lookup_table_groupe4, &gmask, x, y, DIR_HORIZ);
		}
	}
	
	// 2. Vertical (stride = 20)
	for (int x = 0; x < Traits::BOARD_SIZE; x++)
	{
		for (int y = 0; y <= Traits::BOARD_SIZE - 5; y++)
		{
			t_PatternGroup4<Traits> gmask = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i * Traits::STRIDE;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.masks[0][idx] |= (1ULL << offset);
				gmask.masks[1][idx] |= (1ULL << offset);
				gmask.masks[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit_generic<Traits>(gmask.masks[i], x, y + 1 + i);
				gmask.hole_pos[i] = index_bb_generic<Traits>(x, y + 1 + i);
			}
			add_pattern_group4<Traits>(lookup_table_groupe4, &gmask, x, y, DIR_VERT);
		}
	}
	
	// // 3. Diagonale \ (stride = 21)
	for (int y = 0; y <= Traits::BOARD_SIZE - 5; y++)
	{
		for (int x = 0; x <= Traits::BOARD_SIZE - 5; x++)
		{
			t_PatternGroup4<Traits> gmask = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i * Traits::STRIDE_D;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.masks[0][idx] |= (1ULL << offset);
				gmask.masks[1][idx] |= (1ULL << offset);
				gmask.masks[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit_generic<Traits>(gmask.masks[i], x + 1 + i, y + 1 + i);
				gmask.hole_pos[i] = index_bb_generic<Traits>(x + 1 + i, y + 1 + i);
			}
			add_pattern_group4<Traits>(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_D);
		}
	}
	
	// // 4. Diagonale / (stride = 19)
	for (int y = 0; y <= Traits::BOARD_SIZE - 5; y++)
	{
		for (int x = 4; x < Traits::BOARD_SIZE; x++)
		{
			t_PatternGroup4<Traits> gmask = {};
			int start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 5; i++)
			{
				int bit = start_pos + i * Traits::STRIDE_G;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.masks[0][idx] |= (1ULL << offset);
				gmask.masks[1][idx] |= (1ULL << offset);
				gmask.masks[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit_generic<Traits>(gmask.masks[i], x - 1 - i, y + 1 + i);
				gmask.hole_pos[i] = index_bb_generic<Traits>(x - 1 - i, y + 1 + i);
			}
			add_pattern_group4<Traits>(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_G);
		}
	}
}

template<typename Traits>
int check_four_align(const t_PatternList_Groupe4<Traits> lookup_table[Traits::CELL_COUNT][4],
						const typename Traits::Bitboard &boardA, const typename Traits::Bitboard &boardB,
						const int x, const int y)
{
	const int idx = idx_generic<Traits>(x, y);  // Précalculé

	for (int dir = 0; dir < 4; dir++) // boucle des directions
	{
		const t_PatternList_Groupe4<Traits> &list_groupe = lookup_table[idx][dir];
		int count = list_groupe.count;
		
		for (int i = 0; i < count; i++) // boucle des positions relatives
		{
			const t_PatternGroup4<Traits>& groupe = list_groupe.patterns[i];

			if (!get_bb_generic<Traits>(boardB, groupe.hole_pos[0]) && match_pattern(groupe.masks[0], boardA))
				return 1;
			if (!get_bb_generic<Traits>(boardB, groupe.hole_pos[1]) && match_pattern(groupe.masks[1], boardA))
				return 2;
			if (!get_bb_generic<Traits>(boardB, groupe.hole_pos[2]) && match_pattern(groupe.masks[2], boardA))
				return 3;
		}
	}
	return 0;
}

template<typename Traits>
static void	add_pattern_group3_to_lookup(t_PatternList_Groupe3 lookup_table3[Traits::CELL_COUNT][4],
										t_PatternGroupe3 *group, int dir)
{
	// Parcours des 4 positions de pierres (certaines peuvent être -1 si hors plateau)
    for (int i = 0; i < 4; i++)
	{
        int pos = group->stone_pos[i];
        if (pos == -1)
			continue; // position invalide, on ignore
        
        // Conversion position absolue (avec padding 1 bit) -> coordonnées plateau logique
        int x = pos % Traits::STRIDE;
        int y = pos / Traits::STRIDE;
        int cell = idx_generic<Traits>(x, y);
        
        t_PatternList_Groupe3 *list = &lookup_table3[cell][dir];
        if (list->count < 4)
		{
            list->patterns[list->count] = *group; // copie du pattern
            list->count++;
        }
    }

}

template<typename Traits>
void	build_lookup_table3(t_PatternList_Groupe3 lookup_table3[Traits::CELL_COUNT][4])
{
	memset(lookup_table3, 0, sizeof(t_PatternList_Groupe3) * Traits::CELL_COUNT * 4);

	// 1. Horizontal (stride = 1)
	for (int y = 0; y < Traits::BOARD_SIZE; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 2; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * Traits::STRIDE + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i;
				if (x + i >= Traits::BOARD_SIZE)
				{
					pattern.stone_pos[i] = -1;
					// pattern.count = 1;
				}
				
			}
			pattern.hole_pos[0] = start_pos + 1;
			pattern.hole_pos[1] = start_pos + 2;
			pattern.oposant_pos[0] = (x - 2 >= 0) ? (start_pos - 2) : -1;
			pattern.oposant_pos[1] = (x - 1 >= 0) ? (start_pos - 1) : -1;
			pattern.oposant_pos[2] = (x + 3 < Traits::BOARD_SIZE) ? (start_pos + 3) : -1;
			pattern.oposant_pos[3] = (x + 4 < Traits::BOARD_SIZE) ? (start_pos + 4) : -1;
			pattern.oposant_pos[4] = (x + 5 < Traits::BOARD_SIZE) ? (start_pos + 5) : -1;
			add_pattern_group3_to_lookup<Traits>(lookup_table3, &pattern, DIR_HORIZ);
		}
	}

	// 2. Vertical (stride = Traits::STRIDE)
	for (int y = 0; y < Traits::BOARD_SIZE - 2; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * Traits::STRIDE + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i * Traits::STRIDE;
				if (y + i >= Traits::BOARD_SIZE)
				{
					pattern.stone_pos[i] = -1;
					// pattern.count = 1;
				}
				// if  (i == 3)
				// 	pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1 * Traits::STRIDE;
			pattern.hole_pos[1] = start_pos + 2 * Traits::STRIDE;
			pattern.oposant_pos[0] = (y - 2 >= 0) ? (start_pos - 2 * Traits::STRIDE) : -1;
			pattern.oposant_pos[1] = (y - 1 >= 0) ? (start_pos - 1 * Traits::STRIDE) : -1;
			pattern.oposant_pos[2] = (y + 3 < Traits::BOARD_SIZE) ? (start_pos + 3 * Traits::STRIDE) : -1;
			pattern.oposant_pos[3] = (y + 4 < Traits::BOARD_SIZE) ? (start_pos + 4 * Traits::STRIDE) : -1;
			pattern.oposant_pos[4] = (y + 5 < Traits::BOARD_SIZE) ? (start_pos + 5 * Traits::STRIDE) : -1;
			add_pattern_group3_to_lookup<Traits>(lookup_table3, &pattern, DIR_VERT);
		}
	}

	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y < Traits::BOARD_SIZE - 2; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 2; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * Traits::STRIDE + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i * Traits::STRIDE_D;
				if (y + i >= Traits::BOARD_SIZE || x + i >= Traits::BOARD_SIZE)
				{
					pattern.stone_pos[i] = -1;
					// pattern.count = 1;
				}
				// if  (i == 3)
				// 	pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1 * Traits::STRIDE_D;
			pattern.hole_pos[1] = start_pos + 2 * Traits::STRIDE_D;
			pattern.oposant_pos[0] = (y - 2 >= 0 && x - 2 >= 0) ? (start_pos - 2 * Traits::STRIDE_D) : -1;
			pattern.oposant_pos[1] = (y - 1 >= 0 && x - 1 >= 0) ? (start_pos - 1 * Traits::STRIDE_D) : -1;
			pattern.oposant_pos[2] = (y + 3 < Traits::BOARD_SIZE && x + 3 < Traits::BOARD_SIZE) ? (start_pos + 3 * Traits::STRIDE_D) : -1;
			pattern.oposant_pos[3] = (y + 4 < Traits::BOARD_SIZE && x + 4 < Traits::BOARD_SIZE) ? (start_pos + 4 * Traits::STRIDE_D) : -1;
			pattern.oposant_pos[4] = (y + 5 < Traits::BOARD_SIZE && x + 5 < Traits::BOARD_SIZE) ? (start_pos + 5 * Traits::STRIDE_D) : -1;
			add_pattern_group3_to_lookup<Traits>(lookup_table3, &pattern, DIR_DIAG_G);
		}
	}

	// 4. Diagonale / (stride = Traits::BOARD_SIZE)
	for (int y = 0; y < Traits::BOARD_SIZE - 2; y++)
	{
		for (int x = 2; x < Traits::BOARD_SIZE; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * Traits::STRIDE + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i * Traits::STRIDE_G;
				if (y + i >= Traits::BOARD_SIZE || x - i >= Traits::BOARD_SIZE)
				{
					pattern.stone_pos[i] = -1;
					// pattern.count = 1;
				}
				// if  (i == 3)
				// 	pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1 * Traits::STRIDE_G;
			pattern.hole_pos[1] = start_pos + 2 * Traits::STRIDE_G;
			pattern.oposant_pos[0] = (y - 2 >= 0 && x + 2 < Traits::BOARD_SIZE) ? (start_pos - 2 * Traits::STRIDE_G) : -1;
			pattern.oposant_pos[1] = (y - 1 >= 0 && x + 1 < Traits::BOARD_SIZE) ? (start_pos - 1 * Traits::STRIDE_G) : -1;
			pattern.oposant_pos[2] = (y + 3 < Traits::BOARD_SIZE && x - 3 >= 0) ? (start_pos + 3 * Traits::STRIDE_G) : -1;
			pattern.oposant_pos[3] = (y + 4 < Traits::BOARD_SIZE && x - 4 >= 0) ? (start_pos + 4 * Traits::STRIDE_G) : -1;
			pattern.oposant_pos[4] = (y + 5 < Traits::BOARD_SIZE && x - 5 >= 0) ? (start_pos + 5 * Traits::STRIDE_G) : -1;
			add_pattern_group3_to_lookup<Traits>(lookup_table3, &pattern, DIR_DIAG_D);
		}
	}
}



template<typename Traits>
int	check_three_align(const t_PatternList_Groupe3 lookup_table[Traits::CELL_COUNT][4],
						const typename Traits::Bitboard &boardA, const typename Traits::Bitboard &boardB,
						const int x, const int y)
{
	const int	idx = idx_generic<Traits>(x, y);
	int			total_score = 0;
	int			double_three = 0;

	for (int dir = 0; dir < 4; dir++) // parcour des 4 directions
	{
		const t_PatternList_Groupe3	&list_groupe = lookup_table[idx][dir];
		const int	count = list_groupe.count;

		for (int pos = 0; pos < count; pos++)
		{
			// cas 00xxx00					valeur max : vaut 8
			// 10xxx00, 00xx01				valeur moyen, same: oposant_pos[0] et [3] = 128
			// 10xxx01, 01xxx00	 00xxx10	valeur min, same: oposant_pos[1] et [2] = 256

			// 00X0XX0 et 00XX0X0			valeur max: vaut 12
			// 10X0XX0, 00X0XX1, 01XX0X0, 00XX0X1	valeur moyen
			// 10X0XX1 								valeur min

			// double three : on divise la valeur par 4;
			const t_PatternGroupe3& pattern = list_groupe.patterns[pos];
			int		score = 0;
			score  = 0;

			if (pattern.oposant_pos[1] == -1 || get_bb_flate<Traits>(boardB, pattern.oposant_pos[1]))
				score = 256;
			else if (pattern.oposant_pos[0] == -1 || get_bb_flate<Traits>(boardB, pattern.oposant_pos[0]))
				score = 128;
			if (pattern.oposant_pos[2] == -1 || get_bb_flate<Traits>(boardB, pattern.oposant_pos[2]))
				score += 256;
			else if (pattern.oposant_pos[3] == -1 || get_bb_flate<Traits>(boardB, pattern.oposant_pos[3]))
				score += 128;
			
			if (score > 256)
			{
				continue; // alignement de 3 pierre complètement fermé, on ignore
			}	
			if (get_bb_generic<Traits>(boardA, pattern.stone_pos[0]) && 
				get_bb_generic<Traits>(boardA, pattern.stone_pos[1]) &&
				get_bb_generic<Traits>(boardA, pattern.stone_pos[2]))
			{
				// std::cout << "alignement de 3 pierre complètement ouvert, on ignore, score = " << score << std::endl;
				total_score += score + 8;
				double_three += 1;
				continue;
			}
			if (pattern.oposant_pos[3] == -1 || get_bb_flate<Traits>(boardB, pattern.oposant_pos[3]))
				score += 128;
			else if (pattern.oposant_pos[4] == -1 || get_bb_flate<Traits>(boardB, pattern.oposant_pos[4]))
				score += 128;
			if (score > 256)
			{
				continue; // alignement de 3 pierre complètement fermé, on ignore
			}

			if (pattern.stone_pos[3] == -1)
				continue;
			// std::cout << "pattern: hole pos[0] = " << pattern.hole_pos[0] << "hole pos[1] = " << pattern.hole_pos[1] << ", stone pos = " << pattern.stone_pos[0] << std::endl;
			if (!get_bb_generic<Traits>(boardB, pattern.hole_pos[0]) && get_bb_flate<Traits>(boardA, pattern.stone_pos[0]) && get_bb_flate<Traits>(boardA, pattern.stone_pos[2]) &&
				get_bb_generic<Traits>(boardA, pattern.stone_pos[3]))
			{
				total_score += score + 12; // a change
				double_three += 1;
				// std::cout << "alignement de 3 pierre avec troue, on ignore, score = " << score << std::endl;
				continue;
			}
			if (!get_bb_generic<Traits>(boardB, pattern.hole_pos[1]) && get_bb_flate<Traits>(boardA, pattern.stone_pos[0]) && get_bb_flate<Traits>(boardA, pattern.stone_pos[1]) &&
				get_bb_generic<Traits>(boardA, pattern.stone_pos[3]))
			{
				total_score += score + 12;
				double_three += 1;
				// std::cout << "alignement de 3 pierre avec troue 2, on ignore, score = " << score << std::endl;
				continue;
			}
		}
		if (double_three == 2)
			return total_score / 4;
	}
	if (double_three == 0)
		return 0;
	return total_score;
}

//---------------------------------------------------------------------------
template<typename Traits>
static void	add_pattern_super4(t_PatternList_super4<Traits> lookup_table_super4[Traits::CELL_COUNT][4], t_super4<Traits> *pattern, int start_x, int start_y, int dir)
{
	for (int i = 0; i < 7; i++)
	{
		int case_x, case_y;
		switch (dir)
		{
			case DIR_HORIZ:
				case_x = start_x + i;
				case_y = start_y;
				break;
			case DIR_VERT:
				case_x = start_x;
				case_y = start_y + i;
				break;
			case DIR_DIAG_G:
				case_x = start_x - i;
				case_y = start_y + i;
				break;
			case DIR_DIAG_D:
				case_x = start_x + i;
				case_y = start_y + i;
				break;
		}
		// Sécurité : on reste dans le plateau
		if (case_x < 0 || case_x >= Traits::BOARD_SIZE || case_y < 0 || case_y >= Traits::BOARD_SIZE)
			continue;

		int cell = idx_generic<Traits>(case_x, case_y);
		t_PatternList_super4<Traits> *list = &lookup_table_super4[cell][dir];

		if (list->count < 7)
		{
			list->patterns[list->count] = *pattern; // copie du pattern
			list->count++;
		}
	}
}

template<typename Traits>
void build_lookup_table_super4(t_PatternList_super4<Traits> lookup_table[Traits::CELL_COUNT][4])
{
	memset(lookup_table, 0, sizeof(t_PatternList_super4<Traits>) * Traits::CELL_COUNT * 4);
	// horizontal
	for (int y = 0; y < Traits::BOARD_SIZE; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 6; x++)
		{
			t_super4<Traits>	pattern = {};
			int			start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 7; i++)
			{
				int	bit = start_pos + i;
				int	idx = bit >> 6;
				int	offset = bit & 63;

				pattern.mask[idx] |= (1ULL << offset);
			}
			clear_bit_generic<Traits>(pattern.mask, x + 1, y);
			pattern.hole_pos[0] = index_bb_generic<Traits>(x + 1, y);
			clear_bit_generic<Traits>(pattern.mask, x + 5, y);
			pattern.hole_pos[1] = index_bb_generic<Traits>(x + 5, y);
			add_pattern_super4<Traits>(lookup_table, &pattern, x, y, DIR_HORIZ);
		}
	}

	// vertical
	for (int y = 0; y < Traits::BOARD_SIZE - 6; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE; x++)
		{
			t_super4<Traits>	pattern = {};
			int			start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 7; i++)
			{
				int	bit = start_pos + i * Traits::STRIDE;
				int	idx = bit >> 6;
				int	offset = bit & 63;

				pattern.mask[idx] |= (1ULL << offset);
			}
			clear_bit_generic<Traits>(pattern.mask, x, y + 1);
			pattern.hole_pos[0] = index_bb_generic<Traits>(x, y + 1);
			clear_bit_generic<Traits>(pattern.mask, x, y + 5);
			pattern.hole_pos[1] = index_bb_generic<Traits>(x, y + 5);
			add_pattern_super4<Traits>(lookup_table, &pattern, x, y, DIR_VERT);
		}
	}

	// diagonal \ (stride = 21)
	for (int y = 0; y < Traits::BOARD_SIZE - 6; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 6; x++)
		{
			t_super4<Traits>	pattern = {};
			int			start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 7; i++)
			{
				int	bit = start_pos + i * Traits::STRIDE_D;
				int	idx = bit >> 6;
				int	offset = bit & 63;

				pattern.mask[idx] |= (1ULL << offset);
			}
			clear_bit_generic<Traits>(pattern.mask, x + 1, y + 1);
			pattern.hole_pos[0] = index_bb_generic<Traits>(x + 1, y + 1);
			clear_bit_generic<Traits>(pattern.mask, x + 5, y + 5);
			pattern.hole_pos[1] = index_bb_generic<Traits>(x + 5, y + 5);
			add_pattern_super4<Traits>(lookup_table, &pattern, x, y, DIR_DIAG_D);
		}
	}

	// diagonal / (stride = Traits::BOARD_SIZE)
	for (int y = 0; y < Traits::BOARD_SIZE - 6; y++)
	{
		for (int x = 6; x < Traits::BOARD_SIZE; x++)
		{
			t_super4<Traits>	pattern = {};
			int	start_pos = y * Traits::STRIDE + x;

			for (int i = 0; i < 7; i++)
			{
				int	bit = start_pos + i * Traits::BOARD_SIZE;
				int	idx = bit >> 6;
				int	offset = bit & 63;

				pattern.mask[idx] |= (1ULL << offset);
			}
			clear_bit_generic<Traits>(pattern.mask, x - 1, y + 1);
			pattern.hole_pos[0] = index_bb_generic<Traits>(x - 1, y + 1);
			clear_bit_generic<Traits>(pattern.mask, x - 5, y + 5);
			pattern.hole_pos[1] = index_bb_generic<Traits>(x - 5, y + 5);
			add_pattern_super4<Traits>(lookup_table, &pattern, x, y, DIR_DIAG_G);
		}
	}
}


template<typename Traits>
int	check_super4(const t_PatternList_super4<Traits> lookup_table[Traits::CELL_COUNT][4],
					const typename Traits::Bitboard &boardA, const typename Traits::Bitboard &boardB,
					const int x, const int y)
{
	const int	idx = idx_generic<Traits>(x, y);

	for (int dir = 0; dir < 4; dir++)
	{
		const t_PatternList_super4<Traits>	&list_super4 = lookup_table[idx][dir];
		int count = list_super4.count;

		for (int i = 0; i < count; i++)
		{
			const t_super4<Traits>& pattern = list_super4.patterns[i];

			if (match_pattern<Traits>(pattern.mask, boardA) &&
				!get_bb_generic<Traits>(boardB, pattern.hole_pos[0]) &&
				!get_bb_generic<Traits>(boardB, pattern.hole_pos[1]))
			{
				return 1; // super four détecté
			}
		}
	}
	return 0;
}

//---------------------------------------------------------------------------

template<typename Traits>
static void	add_pattern_cross_to_lookup(t_PatternList_Cross lookup_table[Traits::CELL_COUNT], t_cross *pattern)
{
	int positions[5] = {pattern->middle, pattern->up, pattern->down, pattern->left, pattern->right};

	for (int i = 0; i < 5; i++)
	{
		int pos = positions[i];

		int x = pos % Traits::STRIDE;
		int y = pos / Traits::STRIDE;
		int cell = idx_generic<Traits>(x, y);

		t_PatternList_Cross *list = &lookup_table[cell];
		if (list->count < 5)
		{
			list->cross[list->count] = *pattern;
			list->count++;
		}
	}
}

template<typename Traits>
void	build_lookup_table_cross(t_PatternList_Cross lookup_table[Traits::CELL_COUNT])
{
	memset(lookup_table, 0, sizeof(t_PatternList_Cross) * Traits::CELL_COUNT);

	for (int y = 1; y < Traits::BOARD_SIZE - 1; y++)
	{
		for (int x = 1; x < Traits::BOARD_SIZE - 1; x++)
		{
			t_cross	pattern = {};
			int		start_pos = y * Traits::STRIDE + x; // position de la pierre centrale du cross

			pattern.middle = start_pos;
			pattern.up = start_pos - Traits::STRIDE;
			pattern.down = start_pos + Traits::STRIDE;
			pattern.left = start_pos - 1;
			pattern.right = start_pos + 1;

			pattern.opposant_up[0] = (y - 3 >= 0) ? (start_pos - 3 * Traits::STRIDE) : -1;
			pattern.opposant_up[1] = (y - 2 >= 0) ? (start_pos - 2 * Traits::STRIDE) : -1;

			pattern.opposant_down[0] = (y + 3 < Traits::BOARD_SIZE) ? (start_pos + 3 * Traits::STRIDE) : -1;
			pattern.opposant_down[1] = (y + 2 < Traits::BOARD_SIZE) ? (start_pos + 2 * Traits::STRIDE) : -1;

			pattern.opposant_left[0] = (x - 3 >= 0) ? (start_pos - 3) : -1;
			pattern.opposant_left[1] = (x - 2 >= 0) ? (start_pos - 2) : -1;

			pattern.opposant_right[0] = (x + 3 < Traits::BOARD_SIZE) ? (start_pos + 3) : -1;
			pattern.opposant_right[1] = (x + 2 < Traits::BOARD_SIZE) ? (start_pos + 2) : -1;
			add_pattern_cross_to_lookup<Traits>(lookup_table, &pattern);
		}
	}
}


template<typename Traits>
int	check_cross(const t_PatternList_Cross lookup_table[Traits::CELL_COUNT], 
					const typename Traits::Bitboard &boardA, const typename Traits::Bitboard &boardB,
					const int x, const int y)
{
	const int	idx = idx_generic<Traits>(x, y);
	const	t_PatternList_Cross	&list_cross = lookup_table[idx];

	for (int p = 0; p < list_cross.count; p++)
	{
		const t_cross	&cross = list_cross.cross[p];
		int			opposant_score = 0;
		int			score = 0;

		if (get_bb_generic<Traits>(boardB, cross.middle) ||
			get_bb_generic<Traits>(boardB, cross.up) || get_bb_flate<Traits>(boardB, cross.down) ||
			get_bb_generic<Traits>(boardB, cross.left) || get_bb_flate<Traits>(boardB, cross.right))
			continue;

		if (cross.opposant_up[1] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_up[1]))
			opposant_score = 64;
		else if (cross.opposant_up[0] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_up[0]))
			opposant_score = 32;
		if (cross.opposant_down[1] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_down[1]))
			opposant_score += 64;
		else if (cross.opposant_down[0] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_down[0]))
			opposant_score += 32;
		if (cross.opposant_left[1] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_left[1]))
			opposant_score += 64;
		else if (cross.opposant_left[0] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_left[0]))
			opposant_score += 32;
		if (cross.opposant_right[1] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_right[1]))
			opposant_score += 64;
		else if (cross.opposant_right[0] == -1 || get_bb_flate<Traits>(boardB, cross.opposant_right[0]))
			opposant_score += 32;

		if (opposant_score > 64)
			continue;

		if (get_bb_generic<Traits>(boardA, cross.middle))
			score += 3;
		if (get_bb_generic<Traits>(boardA, cross.up))
			score += 4;
		if (get_bb_generic<Traits>(boardA, cross.down))
			score += 4;
		if (get_bb_generic<Traits>(boardA, cross.left))
			score += 4;
		if (get_bb_generic<Traits>(boardA, cross.right))
			score += 4;
		
		if (score >= 15) // on a au moins une croix partiel
			return score + opposant_score;
	}
	return 0;
}




// void	test_bitboard(const GameBoard& board, int x, int y); // Fonction de test pour vérifier les patterns sur le bitboard
		
//---------------------------------------------------------------------------

// Liste des valeurs de retour pour la fonction check_three_align:

// 0 : pas d'alignement de 3 pierres

#define SCORE_3_FULL		8	// 3 pierres pleines (sans trou)
#define SCORE_3_HOLE		12	// 3 pierres avec un trou

// Modificateurs pour présence de pierre adverse
#define SCORE_OPP_EXTERN	128	// adverse à une extrémité externe (loin des pierres)
#define SCORE_OPP_INTERN	256	// adverse à une extrémité interne (coller aux pierres)

// Scores combinés pour un alignement simple
#define SCORE_FULL_EXTERN	(SCORE_3_FULL + SCORE_OPP_EXTERN)	// 136
#define SCORE_HOLE_EXTERN	(SCORE_3_HOLE + SCORE_OPP_EXTERN)	// 140

#define SCORE_FULL_INTERN	(SCORE_3_FULL + SCORE_OPP_INTERN)	// 264
#define SCORE_HOLE_INTERN	(SCORE_3_HOLE + SCORE_OPP_INTERN)	// 268

// Scores pour double three (somme / 4)
#define SCORE_DOUBLE_FULL_FULL			4	// (8+8)/4
#define SCORE_DOUBLE_HOLE_FULL			5	// (12+8)/4
#define SCORE_DOUBLE_HOLE_HOLE			6	// (12+12)/4

#define SCORE_DOUBLE_FULL_FULL_EXTERN	36	// (128+8+8)/4 = 144/4 = 36
#define SCORE_DOUBLE_HOLE_FULL_EXTERN	37	// (128+12+8)/4 = 148/4 = 37
#define SCORE_DOUBLE_HOLE_HOLE_EXTERN	38	// (128+12+12)/4 = 152/4 = 38

#define SCORE_DOUBLE_FULL_FULL_INTERN	68	// (256+8+8)/4 = 272/4 = 68
#define SCORE_DOUBLE_HOLE_FULL_INTERN	69	// (256+12+8)/4 = 276/4 = 69
#define SCORE_DOUBLE_HOLE_HOLE_INTERN	70	// (256+12+12)/4 = 280/4 = 70

#define SCORE_DOUBLE_FULL_FULL_MIXED	100	// (256+128+8+8)/4 = 400/4 = 100
#define SCORE_DOUBLE_HOLE_FULL_MIXED	101	// (256+128+12+8)/4 = 404/4 = 101
#define SCORE_DOUBLE_HOLE_HOLE_MIXED	102	// (256+128+12+12)/4 = 408/4 = 102

#define SCORE_DOUBLE_FULL_FULL_INTERN2	132	// (256+256+8+8)/4 = 528/4 = 132
#define SCORE_DOUBLE_HOLE_FULL_INTERN2	133	// (256+256+12+8)/4 = 532/4 = 133
#define SCORE_DOUBLE_HOLE_HOLE_INTERN2	134	// (256+256+12+12)/4 = 536/4 = 134


// Liste des valeurs de retour pour la fonction check_cross:

// Patterns de base
#define CROSS_NONE				0
#define CROSS_DEMI_NO_MID		16	// demi-croix sans trou central
#define CROSS_DEMI_MID			15	// demi-croix avec trou central
#define CROSS_FULL				19	// croix complète

// Modificateurs adverses
#define CROSS_OPP_EXTERN		32	// adversaire à une extrémité
#define CROSS_OPP_INTERN		64	// adversaire à l'intérieur

// Combinaisons croix complète
#define CROSS_FULL_OPP_EXTERN	(CROSS_FULL + CROSS_OPP_EXTERN)	// 51
#define CROSS_FULL_OPP_INTERN	(CROSS_FULL + CROSS_OPP_INTERN)	// 83

// Combinaisons demi-croix sans trou central
#define CROSS_DEMI_NO_OPP_EXTERN	(CROSS_DEMI_NO_MID + CROSS_OPP_EXTERN)	// 48
#define CROSS_DEMI_NO_OPP_INTERN	(CROSS_DEMI_NO_MID + CROSS_OPP_INTERN)	// 80

// Combinaisons demi-croix avec trou central
#define CROSS_DEMI_MID_OPP_EXTERN	(CROSS_DEMI_MID + CROSS_OPP_EXTERN)	// 47
#define CROSS_DEMI_MID_OPP_INTERN	(CROSS_DEMI_MID + CROSS_OPP_INTERN)	// 79


#endif // PATTERN_HPP

		
// liste des patternes
	// 1. alignement de 5 pierres (gagne)
	// 2. alignement de 4 pierres partiellement ouvert (open four)
	// 3. alignement de 4 pierres completement ouvert (open four)
	// 4. alignement de 4 pierres troue (broken four)
	// [ ] [x] [x] [x] [x] [ ]  -> open four
	// [A] [x] [x] [x] [x] [ ]  -> partiel four
	// [ ] [x] [x] [x] [x] [A]  -> partiel four

	// [X] [ ] [x] [x] [x]  -> partiel four
	// [X] [X] [ ] [x] [x]  -> partiel four
	// [X] [X] [x] [ ] [x]  -> partiel four

	// [X] [ ] [X] [X] [x] [ ] [x]  -> open four indispendable 
	// [X] [ ] [x] [x] [x] [ ] [ ]  -> proto open four inutile
	// [ ] [ ] [X] [X] [x] [ ] [x]  -> proto open four inutile

	// 5. alignement de 3 pierres partiellement ouvert (open three)
	// 6. alignement de 3 pierres completement ouvert (open three)
	// 7. alignement de 3 pierres troue (broken three)
	// [ ] [x] [x] [x] [ ]  -> open three
	// [ ] [ ] [x] [x] [x] [ ] [ ] -> open three ???
	// [A] [ ] [x] [x] [x] [ ] [ ] -> open three ???
	// [ ] [ ] [x] [x] [x] [ ] [A] -> open three ???
	// [A] [ ] [x] [x] [x] [ ] [A] -> open three ???


	// [ ] [A] [x] [x] [x] [ ] [ ]-> partiel three
	// [ ] [ ] [x] [x] [x] [A] [ ]-> partiel three

	// [ ] [X] [ ] [x] [x] [ ]  -> partiel three ???
	// [ ] [x] [x] [ ] [x] [ ]  -> partiel three ???


	// 8 Les paterns en crois. legal si creer grace a capture.
	// [ ] [x] [ ]
	// [x] [x] [x]
	// [ ] [x] [ ]


// 0 : pas d'alignement de 3 pierres

	// [ ] [x] [ ]
	// [x] [ ] [x]
	// [ ] [x] [ ]

      
	// 9. Les paterns en T
	// [ ] [x] [ ]
	// [x] [x] [x]
	// [ ] [ ] [ ]
	// 10. Les paterns en L
	// [x] [ ]
	// [x] [ ]
	// [x] [ ]
	// [ ] [x]


	// double open three, legal seulement si les deux open three ne se croisent pas
	// ou si il est creer grace a une capture.
	// [A] [A] [ ]  [A] [A] [X]
	// [A] [X] [ ]  [A] [X] [ ]
	// [X] [A] [X]  [X] [A] [X]
	// [A] [A] [X]  [A] [A] [X]


	// T 3 2 : techniquement pas un double open tree
	// [X] [ ] [X]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// the T 4 3, potentiellement legal d'apres le sujet.
	//         [ ]
	// [ ] [X] [X] [X] [] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X]
	//         [X]
	//         [X]
	//         [A]

	// proto double three a demi ouvert
	// [X] [ ] [X]
	//     [X]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// double three a demi ouvert
	//         [ ]
	//         [ ]
	// [ ] [X] [X] [X] [ ] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X] 			si c'est a moi de jouer.
	//         [X]
	//         [A]

	// [ ] [x] [ ]
	// [x] [x] [x]
	// [ ] [ ] [ ]
	// 10. Les paterns en L
	// [x] [ ]
	// [x] [ ]
	// [x] [ ]
	// [ ] [x]


	// double open three, legal seulement si les deux open three ne se croisent pas
	// ou si il est creer grace a une capture.
	// [A] [A] [ ]  [A] [A] [X]
	// [A] [X] [ ]  [A] [X] [ ]
	// [X] [A] [X]  [X] [A] [X]
	// [A] [A] [X]  [A] [A] [X]


	// T 3 2 : techniquement pas un double open tree
	// [X] [ ] [X]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// the T 4 3, potentiellement legal d'apres le sujet.
	//         [ ]
	// [ ] [X] [X] [X] [] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X]
	//         [X]
	//         [X]
	//         [A]

	// proto double three a demi ouvert
	// [X] [ ] [X]
	//     [X]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// double three a demi ouvert
	//         [ ]
	//         [ ]
	// [ ] [X] [X] [X] [ ] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X] 			si c'est a moi de jouer.
	//         [X]
	//         [A]

