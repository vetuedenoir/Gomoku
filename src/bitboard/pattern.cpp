#include "bitboard/pattern.hpp"

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

// Version ultra rapide avec table de lookup, la plus efficace a ce jour

void add_mask_to_lookup5(t_MaskList5 lookup_table5[361][4], bitboard19 all_masks5[MAX_WINNING_MASK],
    const int mask_index, const int start_pos, const int stride)
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
		
		t_MaskList5 *list = &lookup_table5[idx][dir];
		
		for (int w = 0; w < 6; w++) {
			list->masks[list->count][w] = all_masks5[mask_index][w];
		}
		list->count++;
	}
}

void build_lookup_table5(t_MaskList5 lookup_table5[361][4], bitboard19 all_masks5[MAX_WINNING_MASK])
{
	int total_masks5 = 0;

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
			add_mask_to_lookup5(lookup_table5, all_masks5, total_masks5, start_pos, 1);
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
			add_mask_to_lookup5(lookup_table5, all_masks5, total_masks5, start_pos, 20);
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
			add_mask_to_lookup5(lookup_table5, all_masks5, total_masks5, start_pos, 21);
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
			add_mask_to_lookup5(lookup_table5, all_masks5, total_masks5, start_pos, 19);
			total_masks5++;
		}
	}
}


// Vérification ultra rapide
int isWin_ultra(t_MaskList5 lookup_table5[361][4], const bitboard19& bboard, const int x, const int y)
{
	const int idx = IDX(x, y);  // Précalculé

	// t_BWBoard19 board = {};
	
	for (int dir = 0; dir < 4; dir++) {
		t_MaskList5 *list = &lookup_table5[idx][dir];
		int count = list->count;

		for (int i = 0; i < count; i++) {
			uint64_t (*mask)[6] = &list->masks[i];
			// memcpy(board.black, list->masks[i], 6 * sizeof(uint64_t));
			if ((((*mask)[0] & bboard[0]) != (*mask)[0]) ||
			(((*mask)[1] & bboard[1]) != (*mask)[1]) ||
			(((*mask)[2] & bboard[2]) != (*mask)[2]) ||
			(((*mask)[3] & bboard[3]) != (*mask)[3]) ||
			(((*mask)[4] & bboard[4]) != (*mask)[4]) ||
			(((*mask)[5] & bboard[5]) != (*mask)[5]))
			{
				// print_bb_19(board);
				continue;
			}
			return 1;
		}
	}
	return 0;
}


void add_mask_to_lookup4(t_MaskList4 lookup_table4[361][4], bitboard19 all_masks4[MAX_FOUR_MASK],
	const int mask_index, const int start_pos, const int stride, const int left_pos, const int right_pos)
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
		
		t_MaskList4 *list = &lookup_table4[idx][dir];
		
		for (int w = 0; w < 6; w++) {
			list->masks[list->count][w] = all_masks4[mask_index][w];
		}
		list->left_pos[list->count] = left_pos;
		list->right_pos[list->count] = right_pos;
		list->count++;
	}
}


void	build_lookup_table4(t_MaskList4 lookup_table4[361][4], bitboard19 all_masks4[MAX_FOUR_MASK]) {

	int total_masks4 = 0;

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
			add_mask_to_lookup4(lookup_table4, all_masks4, total_masks4, start_pos, 1, left_pos, right_pos);
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
			add_mask_to_lookup4(lookup_table4, all_masks4, total_masks4, start_pos, 20, left_pos, right_pos);
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
			add_mask_to_lookup4(lookup_table4, all_masks4, total_masks4, start_pos, 21, left_pos, right_pos);
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
			add_mask_to_lookup4(lookup_table4, all_masks4, total_masks4, start_pos, 19, left_pos, right_pos);
			total_masks4++;
		}
	}
}

int is_Open_4(t_MaskList4 lookup_table4[361][4], const bitboard19 &boardA, const bitboard19 &boardB, const int x, const int y)
{
	const int idx = IDX(x, y);  // Précalculé
	int	opening_score = 2;

	// si l'opening score est a zero, il n'y a pas d'alignement de 4 pierre
	// ou il y a un alignement mais il est completement ferme.
	// si l'opening score est a 1, c'est que l'alignement de 4 pierre est partiellement ouvert.
	// si le score est  a 2, c'est que c'est un open 4.

	for (int dir = 0; dir < 4; dir++) {
		t_MaskList4 *list = &lookup_table4[idx][dir];
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
			if (list->left_pos[i] == -1 || get_bb19(boardB, list->left_pos[i]))
				opening_score -= 1; 
			if (list->right_pos[i] == -1 || get_bb19(boardB, list->right_pos[i]))
				opening_score -= 1;
			return opening_score;
		}
	}
	return 0;
}


static void add_pattern_group_to_lookup(t_MaskList_Groupe4 lookup_table[361][4],
                                        t_PatternGroup4 *group,
                                        int start_x, int start_y, int dir)
{
    for (int i = 0; i < 5; i++) {
        int case_x, case_y;
        switch (dir) {
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
        if (case_x < 0 || case_x >= 19 || case_y < 0 || case_y >= 19)
            continue;

        int cell = case_y * 19 + case_x;
        t_MaskList_Groupe4 *list = &lookup_table[cell][dir];
        list->masks[list->count] = *group;
        list->count++;
    }
}


void build_lookup_table_groupe4(t_MaskList_Groupe4 lookup_table_groupe4[361][4])
{
	int total_masks4 = 0;

	// 1. Horizontal (stride = 1)
	for (int y = 0; y < 19; y++) {
		for (int x = 0; x <= 14; x++) {
			t_PatternGroup4 gmask = {};
			int start_pos = y * 20 + x;

			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.patterns[0][idx] |= (1ULL << offset);
				gmask.patterns[1][idx] |= (1ULL << offset);
				gmask.patterns[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit(gmask.patterns[i], x + 1 + i, y );
				gmask.hole_pos[i] = index_bb19(x + 1 + i, y);
			}
			add_pattern_group_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_HORIZ);
			total_masks4++;
		}
	}
	
	// 2. Vertical (stride = 20)
	for (int x = 0; x < 19; x++) {
		for (int y = 0; y <= 14; y++) {
			t_PatternGroup4 gmask = {};
			int start_pos = y * 20 + x;

			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i * 20;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.patterns[0][idx] |= (1ULL << offset);
				gmask.patterns[1][idx] |= (1ULL << offset);
				gmask.patterns[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit(gmask.patterns[i], x, y + 1 + i);
				gmask.hole_pos[i] = index_bb19(x, y + 1 + i);
			}
			add_pattern_group_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_VERT);
			total_masks4++;
		}
	}
	
	// // 3. Diagonale \ (stride = 21)
	for (int y = 0; y <= 14; y++) {
		for (int x = 0; x <= 14; x++) {
			t_PatternGroup4 gmask = {};
			int start_pos = y * 20 + x;

			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i * 21;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.patterns[0][idx] |= (1ULL << offset);
				gmask.patterns[1][idx] |= (1ULL << offset);
				gmask.patterns[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit(gmask.patterns[i], x + 1 + i, y + 1 + i);
				gmask.hole_pos[i] = index_bb19(x + 1 + i, y + 1 + i);
			}
			add_pattern_group_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_D);
			total_masks4++;
		}
	}
	
	// // 4. Diagonale / (stride = 19)
	for (int y = 0; y <= 14; y++) {
		for (int x = 4; x < 19; x++) {
			t_PatternGroup4 gmask = {};
			int start_pos = y * 20 + x;

			for (int i = 0; i < 5; i++) {
				int bit = start_pos + i * 19;
				int idx = bit >> 6;
				int offset = bit & 63;
				gmask.patterns[0][idx] |= (1ULL << offset);
				gmask.patterns[1][idx] |= (1ULL << offset);
				gmask.patterns[2][idx] |= (1ULL << offset);
			}
			for (int i = 0; i < 3; i++)
			{
				clear_bit(gmask.patterns[i], x - 1 - i, y + 1 + i);
				gmask.hole_pos[i] = index_bb19(x - 1 - i, y + 1 + i);
			}
			add_pattern_group_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_G);
			total_masks4++;
		}
	}
	// std::cout << "total_masks4: " << total_masks4 << std::endl;
}

static inline bool match_pattern(const bitboard19& pat, const bitboard19& board) {
    return (pat[0] & board[0]) == pat[0] &&
           (pat[1] & board[1]) == pat[1] &&
           (pat[2] & board[2]) == pat[2] &&
           (pat[3] & board[3]) == pat[3] &&
           (pat[4] & board[4]) == pat[4] &&
           (pat[5] & board[5]) == pat[5];
}

int check_four_align(const t_MaskList_Groupe4 lookup_table[361][4], const bitboard19 &boardA, const bitboard19 &boardB,
						const int x, const int y)
{
	const int idx = IDX(x, y);  // Précalculé


	for (int dir = 0; dir < 4; dir++) // boucle des directions
	{
		const t_MaskList_Groupe4 &list_groupe = lookup_table[idx][dir];
		int count = list_groupe.count;
		
		for (int i = 0; i < count; i++) // boucle des positions relatives
		{
			const t_PatternGroup4& groupe = list_groupe.masks[i];

			if (!get_bb19(boardB, groupe.hole_pos[0]) && match_pattern(groupe.patterns[0], boardA))
				return 1;
			if (!get_bb19(boardB, groupe.hole_pos[1]) && match_pattern(groupe.patterns[1], boardA))
				return 2;
			if (!get_bb19(boardB, groupe.hole_pos[2]) && match_pattern(groupe.patterns[2], boardA))
				return 3;
		}
	}
	return 0;
}

static void	add_pattern_group3_to_lookup(t_MaskList_Groupe3 lookup_table3[361][4],
										t_PatternGroupe3 *group, int dir)
{
	// Parcours des 4 positions de pierres (certaines peuvent être -1 si hors plateau)
    for (int i = 0; i < 4; i++) {
        int pos = group->stone_pos[i];
        if (pos == -1) // petit doute sur la validité de cette condition, a tester
			continue; // position invalide, on ignore
        
        // Conversion position absolue (avec padding 1 bit) -> coordonnées plateau logique
        int x = pos % 20;
        int y = pos / 20;
        int cell = y * 19 + x;   // 0..360
        
        t_MaskList_Groupe3 *list = &lookup_table3[cell][dir];
        if (list->count < 4) {
            list->patterns[list->count] = *group; // copie du pattern
            list->count++;
        }
    }

}

void	build_lookup_table3(t_MaskList_Groupe3 lookup_table3[361][4]) {
	int total_patterns3 = 0;

	// 1. Horizontal (stride = 1)
	for (int y = 0; y < 19; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * 20 + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i;
				if (x + i == 19)
				{
					pattern.stone_pos[i] = -1;
					pattern.count = 1;
				}
				if  (i == 3)
					pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1;
			pattern.hole_pos[1] = start_pos + 2;
			pattern.oposant_pos[0] = (x - 2 >= 0) ? (start_pos - 2) : -1;
			pattern.oposant_pos[1] = (x - 1 >= 0) ? (start_pos - 1) : -1;
			pattern.oposant_pos[2] = (x + 3 < 19) ? (start_pos + 3) : -1;
			pattern.oposant_pos[3] = (x + 4 < 19) ? (start_pos + 4) : -1;
			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_HORIZ);
			total_patterns3++;
		}
	}

	// 2. Vertical (stride = 20)
	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 19; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * 20 + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i * 20;
				if (y + i == 19)
				{
					pattern.stone_pos[i] = -1;
					pattern.count = 1;
				}
				if  (i == 3)
					pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1 * 20;
			pattern.hole_pos[1] = start_pos + 2 * 20;
			pattern.oposant_pos[0] = (y - 2 >= 0) ? (start_pos - 2 * 20) : -1;
			pattern.oposant_pos[1] = (y - 1 >= 0) ? (start_pos - 1 * 20) : -1;
			pattern.oposant_pos[2] = (y + 3 < 19) ? (start_pos + 3 * 20) : -1;
			pattern.oposant_pos[3] = (y + 4 < 19) ? (start_pos + 4 * 20) : -1;
			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_VERT);
			total_patterns3++;
		}
	}

	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * 20 + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i * 21;
				if (y + i == 19)
				{
					pattern.stone_pos[i] = -1;
					pattern.count = 1;
				}
				if  (i == 3)
					pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1 * 21;
			pattern.hole_pos[1] = start_pos + 2 * 21;
			pattern.oposant_pos[0] = (y - 2 >= 0 && x - 2 >= 0) ? (start_pos - 2 * 21) : -1;
			pattern.oposant_pos[1] = (y - 1 >= 0 && x - 1 >= 0) ? (start_pos - 1 * 21) : -1;
			pattern.oposant_pos[2] = (y + 3 < 19 && x + 3 < 19) ? (start_pos + 3 * 21) : -1;
			pattern.oposant_pos[3] = (y + 4 < 19 && x + 4 < 19) ? (start_pos + 4 * 21) : -1;
			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_DIAG_G);
			total_patterns3++;
		}
	}

	// 4. Diagonale / (stride = 19)
	for (int y = 0; y < 16; y++)
	{
		for (int x = 2; x < 19; x++)
		{
			t_PatternGroupe3 pattern = {};
			int start_pos = y * 20 + x;
			
			for (int i = 0; i < 4; i++)
			{
				pattern.stone_pos[i] = start_pos + i * 19;
				if (y + i == 19)
				{
					pattern.stone_pos[i] = -1;
					pattern.count = 1;
				}
				if  (i == 3)
					pattern.count = 3;
			}
			pattern.hole_pos[0] = start_pos + 1 * 19;
			pattern.hole_pos[1] = start_pos + 2 * 19;
			pattern.oposant_pos[0] = (y - 2 >= 0 && x + 2 < 19) ? (start_pos - 2 * 19) : -1;
			pattern.oposant_pos[1] = (y - 1 >= 0 && x + 1 < 19) ? (start_pos - 1 * 19) : -1;
			pattern.oposant_pos[2] = (y + 4 < 19 && x - 4 >= 0) ? (start_pos + 4 * 19) : -1;
			pattern.oposant_pos[3] = (y + 3 < 19 && x - 3 >= 0) ? (start_pos + 3 * 19) : -1;
			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_DIAG_D);
			total_patterns3++;
		}
	}
	std::cout << "total_patterns3: " << total_patterns3 << std::endl;
}

int	check_three_align(const t_MaskList_Groupe3 lookup_table[361][4], const bitboard19 &boardA, const bitboard19 &boardB,
						const int x, const int y)
{
	const int	idx = IDX(x, y);
	int			total_score = 0;
	int			double_three = 0;

	for (int dir = 0; dir < 4; dir++) // parcour des 4 directions
	{
		const t_MaskList_Groupe3	&list_groupe = lookup_table[idx][dir];
		const int	count = list_groupe.count;

		for (int pos = 0; pos < count; pos++)
		{
			// cas 00xxx00					valeur max : vaut 8
			// 10xxx00, 00xx01				valeur moyen, same: oposant_pos[0] et [3] = 128
			// 10xxx01, 01xxx00	 00xxx10	valeur min, same: oposant_pos[1] et [2] = 256

			// 00X0XX0 et 00XX0X0			valeur max: vaut 16
			// 10X0XX0, 00X0XX1, 01XX0X0, 00XX0X1	valeur moyen
			// 10X0XX1 								valeur min

			// double three : on divise la valeur par 4;
			const t_PatternGroupe3& pattern = list_groupe.patterns[pos];
			int		score = 0;
			score  = 0;

			if (pattern.oposant_pos[1] == -1 || get_bb19(boardB, pattern.oposant_pos[1]))
				score = 256;
			else if (pattern.oposant_pos[0] == -1 || get_bb19(boardB, pattern.oposant_pos[0]))
				score = 128;
			if (pattern.oposant_pos[2] == -1 || get_bb19(boardB, pattern.oposant_pos[2]))
				score += 256;
			else if (pattern.oposant_pos[3] == -1 || get_bb19(boardB, pattern.oposant_pos[3]))
				score += 128;
			
			if (score > 256)
			{
					continue; // alignement de 3 pierre complètement fermé, on ignore
			}	
				
			if (get_bb19(boardA, pattern.stone_pos[0]) && 
				get_bb19(boardA, pattern.stone_pos[1]) &&
				get_bb19(boardA, pattern.stone_pos[2]))
			{
				std::cout << "alignement de 3 pierre complètement ouvert, on ignore, score = " << score << std::endl;
				total_score += score + 8;
				double_three += 1;		
					continue;
			}
			if (pattern.stone_pos[3] == -1)
				continue;
			// std::cout << "pattern: hole pos[0] = " << pattern.hole_pos[0] << "hole pos[1] = " << pattern.hole_pos[1] << ", stone pos = " << pattern.stone_pos[0] << std::endl;
			if (!get_bb19(boardB, pattern.hole_pos[0]) && get_bb19(boardA, pattern.stone_pos[0]) && get_bb19(boardA, pattern.stone_pos[2]) &&
				get_bb19(boardA, pattern.stone_pos[3]))
			{
				total_score += score + 16;
				double_three += 1;
				// std::cout << "alignement de 3 pierre avec troue, on ignore, score = " << score << std::endl;
				continue;
			}
			if (!get_bb19(boardB, pattern.hole_pos[1]) && get_bb19(boardA, pattern.stone_pos[0]) && get_bb19(boardA, pattern.stone_pos[1]) &&
				get_bb19(boardA, pattern.stone_pos[3]))
			{
				total_score += score + 16;
				double_three += 1;
				// std::cout << "alignement de 3 pierre avec troue 2, on ignore, score = " << score << std::endl;
				continue;
			}
		}
		if (double_three == 2)
			return total_score / 4;
	}
	if (double_three == 0)
		return 100000;
	return total_score;
}

void	test_bitboard(const GameBoard& board, int x, int y)
{
	static bitboard19	all_masks5[MAX_WINNING_MASK] = {};
	static t_MaskList5	lookup_table5[361][4] = {};  // 19*19 = 361 positions possibles, 4 directions

	static bitboard19	all_masks4[MAX_FOUR_MASK] = {};
	static t_MaskList4	lookup_table4[361][4] = {};

	static t_MaskList_Groupe4 lookup_table_groupe4[361][4] = {};

	static t_MaskList_Groupe3 lookup_table3[361][4] = {};

	static bool initialized = false;
	if (!initialized) {
		build_lookup_table5(lookup_table5, all_masks5);
		build_lookup_table4(lookup_table4, all_masks4);
		build_lookup_table_groupe4(lookup_table_groupe4);
		build_lookup_table3(lookup_table3);
		initialized = true;
	}

	t_BWBoard19	bitboard = GameBoard_to_bitboard(board);
	print_bb_19(bitboard);

	std::cout << "test bitboard : x = " << x << " y = " << y << std::endl;


	if (isWin_ultra(lookup_table5, bitboard.black, x, y))
		std::cout << "les noires ont gagne !!!" << std::endl;
	if (isWin_ultra(lookup_table5, bitboard.white, x, y))
		std::cout << "les blanches ont gagne !!!" << std::endl;

	int	black_four = is_Open_4(lookup_table4, bitboard.black, bitboard.white, x, y);
	int white_four = is_Open_4(lookup_table4, bitboard.white, bitboard.black, x, y);
	if (black_four == 1)
		std::cout << "alignement de 4 pierre noir partiellement ouvert" << std::endl;
	else if (black_four == 2)
		std::cout << "Open four black !!!" << std::endl;
	if (white_four == 1)
		std::cout << "alignement de 4 pierre blanche partiellement ouvert" << std::endl;
	else if (white_four == 2)
		std::cout << "Open four white !!!" << std::endl;

	if (check_four_align(lookup_table_groupe4, bitboard.black, bitboard.white, x, y) > 0)
		std::cout << "alignement de 4 pierre noir avec troue" << std::endl;
	else if (check_four_align(lookup_table_groupe4, bitboard.white, bitboard.black, x, y) > 0)
		std::cout << "alignement de 4 pierre blanche avec troue" << std::endl;
	
	int	three_black = check_three_align(lookup_table3, bitboard.black, bitboard.white, x, y);
	int	three_white = check_three_align(lookup_table3, bitboard.white, bitboard.black, x, y);

	std::cout << "valeur de retour, detection three black:  " << three_black << std::endl;
	std::cout << "valeur de retour, detection three white:  " << three_white << std::endl;


}
