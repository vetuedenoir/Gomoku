#include "bitboard/pattern.hpp"

// void	pattern_universel(bitboard19 bb, int size, int pos, int strides)
// {
// 	int	y = pos / 19;
// 	int	x = pos % 19;

// 	int	start = y * 20 + x;
	
// 	for (int i = 0; i < size; i++)
// 	{
// 		int bit = start + i * strides;

// 		int idx = bit / 64;
// 		int	offset = bit % 64;

// 		bb[idx] |= (1ULL << offset);
// 	}
// }

// // Version ultra rapide avec table de lookup, la plus efficace a ce jour


// static inline bool match_pattern(const bitboard19& pat, const bitboard19& board)
// {
//     return (pat[0] & board[0]) == pat[0] &&
//            (pat[1] & board[1]) == pat[1] &&
//            (pat[2] & board[2]) == pat[2] &&
//            (pat[3] & board[3]) == pat[3] &&
//            (pat[4] & board[4]) == pat[4] &&
//            (pat[5] & board[5]) == pat[5];
// }

// static void add_mask_to_lookup5(t_PatternList5 lookup_table5[361][4], const bitboard19 mask, const int start_pos, const int stride)
// {
// 	int dir;

// 	if (stride == 1)
// 		dir = DIR_HORIZ;
// 	else if (stride == 20)
// 		dir = DIR_VERT;
// 	else if (stride == 21)
// 		dir = DIR_DIAG_G;
// 	else if (stride == 19)
// 		dir = DIR_DIAG_D;
// 	else
// 		return;
	
// 	for (int i = 0; i < 5; i++)
// 	{
// 		int pos = start_pos + i * stride;
// 		int y = pos / 20;
// 		int x = pos % 20;
// 		int idx =
		
// 		t_PatternList5 *list = &lookup_table5[idx][dir];
		
// 		for (int w = 0; w < 6; w++)
// 			list->masks[list->count][w] = mask[w];
// 		list->count++;
// 	}
// }


// void build_lookup_table5(t_PatternList5 lookup_table5[361][4])
// {
// 	// 1. Horizontal (stride = 1)
// 	for (int y = 0; y < 19; y++)
// 	{
// 		for (int x = 0; x <= 14; x++)
// 		{
// 			bitboard19 mask = {0};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				mask[idx] |= (1ULL << offset);
// 			}
// 			add_mask_to_lookup5(lookup_table5, mask, start_pos, 1);
// 		}
// 	}
	
// 	// 2. Vertical (stride = 20)
// 	for (int x = 0; x < 19; x++)
// 	{
// 		for (int y = 0; y <= 14; y++)
// 		{
// 			bitboard19 mask = {0};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i * 20;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				mask[idx] |= (1ULL << offset);
// 			}
// 			add_mask_to_lookup5(lookup_table5, mask, start_pos, 20);
// 		}
// 	}
	
// 	// 3. Diagonale \ (stride = 21)
// 	for (int y = 0; y <= 14; y++)
// 	{
// 		for (int x = 0; x <= 14; x++)
// 		{
// 			bitboard19 mask = {0};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i * 21;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				mask[idx] |= (1ULL << offset);
// 			}
// 			add_mask_to_lookup5(lookup_table5, mask, start_pos, 21);
// 		}
// 	}
	
// 	// 4. Diagonale / (stride = 19)
// 	for (int y = 0; y <= 14; y++)
// 	{
// 		for (int x = 4; x < 19; x++)
// 		{
// 			bitboard19 mask = {0};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i * 19;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				mask[idx] |= (1ULL << offset);
// 			}
// 			add_mask_to_lookup5(lookup_table5, mask, start_pos, 19);
// 		}
// 	}
// }


// // Vérification ultra rapide
// int isWin_ultra(t_PatternList5 lookup_table5[361][4], const bitboard19& bboard, const int x, const int y)
// {
// 	const int idx = IDX(x, y);

// 	for (int dir = 0; dir < 4; dir++)
// 	{
// 		t_PatternList5 *list = &lookup_table5[idx][dir];
// 		int count = list->count;

// 		for (int i = 0; i < count; i++)
// 			if (match_pattern(list->masks[i], bboard))
// 				return 1;
// 	}
// 	return 0;
// }


// void add_mask_to_lookup4(t_PatternList4 lookup_table4[361][4], t_Pattern4 *pattern ,
// 	const int start_pos, const int stride)
// {
// 	int dir;

// 	if (stride == 1)
// 		dir = DIR_HORIZ;
// 	else if (stride == 20)
// 		dir = DIR_VERT;
// 	else if (stride == 21)
// 		dir = DIR_DIAG_G;
// 	else if (stride == 19)
// 		dir = DIR_DIAG_D;
// 	else return;

// 	for (int i = 0; i < 4; i++)
// 	{
// 		int pos = start_pos + i * stride;
// 		int y = pos / 20;
// 		int x = pos % 20;
// 		int idx = IDX(x, y);  // Calcul une fois
		
// 		t_PatternList4 *list = &lookup_table4[idx][dir];
		
// 		list->patterns[list->count] = *pattern;
// 		list->count++;
// 	}
// }


// void	build_lookup_table4(t_PatternList4 lookup_table4[361][4])
// {
// 	// 1. Horizontal (stride = 1)
// 	for (int y = 0; y < 19; y++)
// 	{
// 		for (int x = 0; x < 16; x++)
// 		{
// 			t_Pattern4	pattern = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 4; i++)
// 			{
// 				int bit = start_pos + i;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			pattern.opposant_left = (x > 0) ? (start_pos - 1) : -1;
// 			pattern.opposant_right = (x + 4 < 19) ? (start_pos + 4) : -1;

// 			add_mask_to_lookup4(lookup_table4, &pattern, start_pos, 1);
// 		}
// 	}
	
// 	// 2. Vertical (stride = 20)
// 	for (int x = 0; x < 19; x++)
// 	{
// 		for (int y = 0; y < 16; y++)
// 		{
// 			t_Pattern4	pattern = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 4; i++)
// 			{
// 				int bit = start_pos + i * 20;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			pattern.opposant_left = (y > 0) ? (start_pos - 20) : -1;
// 			pattern.opposant_right = (y + 4 < 19) ? (start_pos + 80) : -1;
// 			add_mask_to_lookup4(lookup_table4, &pattern, start_pos, 20);
// 		}
// 	}
	
// 	// 3. Diagonale \ (stride = 21)
// 	for (int y = 0; y < 16; y++)
// 	{
// 		for (int x = 0; x < 16; x++)
// 		{
// 			t_Pattern4	pattern = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 4; i++)
// 			{
// 				int bit = start_pos + i * 21;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			pattern.opposant_left = (y > 0 && x > 0) ? (start_pos - 21) : -1;
// 			pattern.opposant_right = (y + 4 < 19 && x + 4 < 19) ? (start_pos + 84) : -1;
// 			add_mask_to_lookup4(lookup_table4, &pattern, start_pos, 21);
// 		}
// 	}
	
// 	// 4. Diagonale / (stride = 19)
// 	for (int y = 0; y < 16; y++)
// 	{
// 		for (int x = 3; x < 19; x++)
// 		{
// 			t_Pattern4	pattern = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 4; i++)
// 			{
// 				int bit = start_pos + i * 19;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			pattern.opposant_left = (y > 0 && x + 4 < 19) ? (start_pos - 19) : -1;
// 			pattern.opposant_right = (y + 4 < 19 && x > 0) ? (start_pos + 76) : -1;
// 			add_mask_to_lookup4(lookup_table4, &pattern, start_pos, 19);
// 		}
// 	}
// }

// int is_Open_4(t_PatternList4 lookup_table4[361][4], const bitboard19 &boardA, const bitboard19 &boardB, const int x, const int y)
// {
// 	const int idx = IDX(x, y);  // Précalculé
// 	int	opening_score = 2;

// 	// si l'opening score est a zero, il n'y a pas d'alignement de 4 pierre
// 	// ou il y a un alignement mais il est completement ferme.
// 	// si l'opening score est a 1, c'est que l'alignement de 4 pierre est partiellement ouvert.
// 	// si le score est  a 2, c'est que c'est un open 4.

// 	for (int dir = 0; dir < 4; dir++)
// 	{
// 		t_PatternList4 *list = &lookup_table4[idx][dir];
// 		int count = list->count;
		
// 		for (int i = 0; i < count; i++)
// 		{
// 			t_Pattern4 *pattern = &list->patterns[i];
			
// 			if (!match_pattern(pattern->mask, boardA)) // Si le pattern ne match pas, on continue
// 				continue;
// 			if (pattern->opposant_left == -1 || get_bb19(boardB, pattern->opposant_left))
// 				opening_score -= 1; 
// 			if (pattern->opposant_right == -1 || get_bb19(boardB, pattern->opposant_right))
// 				opening_score -= 1;
// 			return opening_score;
// 		}
// 	}
// 	return 0;
// }


// static void add_pattern_group4(t_PatternList_Groupe4 lookup_table[361][4],
//                                         t_PatternGroup4 *group,
//                                         int start_x, int start_y, int dir)
// {
//     for (int i = 0; i < 5; i++)
// 	{
//         int case_x, case_y;
//         switch (dir)
// 		{
//             case DIR_HORIZ:
//                 case_x = start_x + i;
//                 case_y = start_y;
//                 break;
//             case DIR_VERT:
//                 case_x = start_x;
//                 case_y = start_y + i;
//                 break;
//             case DIR_DIAG_G:
//                 case_x = start_x - i;
//                 case_y = start_y + i;
//                 break;
//             case DIR_DIAG_D:
//                 case_x = start_x + i;
//                 case_y = start_y + i;
//                 break;
//         }
//         // Sécurité : on reste dans le plateau
//         if (case_x < 0 || case_x >= 19 || case_y < 0 || case_y >= 19)
//             continue;

//         int cell = IDX(case_x, case_y);
//         t_PatternList_Groupe4 *list = &lookup_table[cell][dir];

//         list->masks[list->count] = *group;
//         list->count++;
//     }
// }


// void build_lookup_table_groupe4(t_PatternList_Groupe4 lookup_table_groupe4[361][4])
// {
// 	// 1. Horizontal (stride = 1)
// 	for (int y = 0; y < 19; y++)
// 	{
// 		for (int x = 0; x <= 14; x++)
// 		{
// 			t_PatternGroup4 gmask = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.masks[0][idx] |= (1ULL << offset);
// 				gmask.masks[1][idx] |= (1ULL << offset);
// 				gmask.masks[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 3; i++)
// 			{
// 				clear_bit(gmask.masks[i], x + 1 + i, y );
// 				gmask.hole_pos[i] = index_bb19(x + 1 + i, y);
// 			}
// 			add_pattern_group4(lookup_table_groupe4, &gmask, x, y, DIR_HORIZ);
// 		}
// 	}
	
// 	// 2. Vertical (stride = 20)
// 	for (int x = 0; x < 19; x++)
// 	{
// 		for (int y = 0; y <= 14; y++)
// 		{
// 			t_PatternGroup4 gmask = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i * 20;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.masks[0][idx] |= (1ULL << offset);
// 				gmask.masks[1][idx] |= (1ULL << offset);
// 				gmask.masks[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 3; i++)
// 			{
// 				clear_bit(gmask.masks[i], x, y + 1 + i);
// 				gmask.hole_pos[i] = index_bb19(x, y + 1 + i);
// 			}
// 			add_pattern_group4(lookup_table_groupe4, &gmask, x, y, DIR_VERT);
// 		}
// 	}
	
// 	// // 3. Diagonale \ (stride = 21)
// 	for (int y = 0; y <= 14; y++)
// 	{
// 		for (int x = 0; x <= 14; x++)
// 		{
// 			t_PatternGroup4 gmask = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i * 21;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.masks[0][idx] |= (1ULL << offset);
// 				gmask.masks[1][idx] |= (1ULL << offset);
// 				gmask.masks[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 3; i++)
// 			{
// 				clear_bit(gmask.masks[i], x + 1 + i, y + 1 + i);
// 				gmask.hole_pos[i] = index_bb19(x + 1 + i, y + 1 + i);
// 			}
// 			add_pattern_group4(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_D);
// 		}
// 	}
	
// 	// // 4. Diagonale / (stride = 19)
// 	for (int y = 0; y <= 14; y++)
// 	{
// 		for (int x = 4; x < 19; x++)
// 		{
// 			t_PatternGroup4 gmask = {};
// 			int start_pos = y * 20 + x;

// 			for (int i = 0; i < 5; i++)
// 			{
// 				int bit = start_pos + i * 19;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.masks[0][idx] |= (1ULL << offset);
// 				gmask.masks[1][idx] |= (1ULL << offset);
// 				gmask.masks[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 3; i++)
// 			{
// 				clear_bit(gmask.masks[i], x - 1 - i, y + 1 + i);
// 				gmask.hole_pos[i] = index_bb19(x - 1 - i, y + 1 + i);
// 			}
// 			add_pattern_group4(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_G);
// 		}
// 	}
// }


// int check_four_align(const t_PatternList_Groupe4 lookup_table[361][4], const bitboard19 &boardA, const bitboard19 &boardB,
// 						const int x, const int y)
// {
// 	const int idx = IDX(x, y);  // Précalculé

// 	for (int dir = 0; dir < 4; dir++) // boucle des directions
// 	{
// 		const t_PatternList_Groupe4 &list_groupe = lookup_table[idx][dir];
// 		int count = list_groupe.count;
		
// 		for (int i = 0; i < count; i++) // boucle des positions relatives
// 		{
// 			const t_PatternGroup4& groupe = list_groupe.masks[i];

// 			if (!get_bb19(boardB, groupe.hole_pos[0]) && match_pattern(groupe.masks[0], boardA))
// 				return 1;
// 			if (!get_bb19(boardB, groupe.hole_pos[1]) && match_pattern(groupe.masks[1], boardA))
// 				return 2;
// 			if (!get_bb19(boardB, groupe.hole_pos[2]) && match_pattern(groupe.masks[2], boardA))
// 				return 3;
// 		}
// 	}
// 	return 0;
// }

// static void	add_pattern_group3_to_lookup(t_PatternList_Groupe3 lookup_table3[361][4],
// 										t_PatternGroupe3 *group, int dir)
// {
// 	// Parcours des 4 positions de pierres (certaines peuvent être -1 si hors plateau)
//     for (int i = 0; i < 4; i++)
// 	{
//         int pos = group->stone_pos[i];
//         if (pos == -1)
// 			continue; // position invalide, on ignore
        
//         // Conversion position absolue (avec padding 1 bit) -> coordonnées plateau logique
//         int x = pos % 20;
//         int y = pos / 20;
//         int cell = IDX(x, y);
        
//         t_PatternList_Groupe3 *list = &lookup_table3[cell][dir];
//         if (list->count < 4)
// 		{
//             list->patterns[list->count] = *group; // copie du pattern
//             list->count++;
//         }
//     }

// }

// void	build_lookup_table3(t_PatternList_Groupe3 lookup_table3[361][4])
// {
// 	int total_patterns3 = 0;

// 	// 1. Horizontal (stride = 1)
// 	for (int y = 0; y < 19; y++)
// 	{
// 		for (int x = 0; x < 17; x++)
// 		{
// 			t_PatternGroupe3 pattern = {};
// 			int start_pos = y * 20 + x;
			
// 			for (int i = 0; i < 4; i++)
// 			{
// 				pattern.stone_pos[i] = start_pos + i;
// 				if (x + i == 19)
// 				{
// 					pattern.stone_pos[i] = -1;
// 					pattern.count = 1;
// 				}
// 				if  (i == 3)
// 					pattern.count = 3;
// 			}
// 			pattern.hole_pos[0] = start_pos + 1;
// 			pattern.hole_pos[1] = start_pos + 2;
// 			pattern.oposant_pos[0] = (x - 2 >= 0) ? (start_pos - 2) : -1;
// 			pattern.oposant_pos[1] = (x - 1 >= 0) ? (start_pos - 1) : -1;
// 			pattern.oposant_pos[2] = (x + 3 < 19) ? (start_pos + 3) : -1;
// 			pattern.oposant_pos[3] = (x + 4 < 19) ? (start_pos + 4) : -1;
// 			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_HORIZ);
// 			total_patterns3++;
// 		}
// 	}

// 	// 2. Vertical (stride = 20)
// 	for (int y = 0; y < 17; y++)
// 	{
// 		for (int x = 0; x < 19; x++)
// 		{
// 			t_PatternGroupe3 pattern = {};
// 			int start_pos = y * 20 + x;
			
// 			for (int i = 0; i < 4; i++)
// 			{
// 				pattern.stone_pos[i] = start_pos + i * 20;
// 				if (y + i == 19)
// 				{
// 					pattern.stone_pos[i] = -1;
// 					pattern.count = 1;
// 				}
// 				if  (i == 3)
// 					pattern.count = 3;
// 			}
// 			pattern.hole_pos[0] = start_pos + 1 * 20;
// 			pattern.hole_pos[1] = start_pos + 2 * 20;
// 			pattern.oposant_pos[0] = (y - 2 >= 0) ? (start_pos - 2 * 20) : -1;
// 			pattern.oposant_pos[1] = (y - 1 >= 0) ? (start_pos - 1 * 20) : -1;
// 			pattern.oposant_pos[2] = (y + 3 < 19) ? (start_pos + 3 * 20) : -1;
// 			pattern.oposant_pos[3] = (y + 4 < 19) ? (start_pos + 4 * 20) : -1;
// 			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_VERT);
// 			total_patterns3++;
// 		}
// 	}

// 	// 3. Diagonale \ (stride = 21)
// 	for (int y = 0; y < 17; y++)
// 	{
// 		for (int x = 0; x < 17; x++)
// 		{
// 			t_PatternGroupe3 pattern = {};
// 			int start_pos = y * 20 + x;
			
// 			for (int i = 0; i < 4; i++)
// 			{
// 				pattern.stone_pos[i] = start_pos + i * 21;
// 				if (y + i == 19)
// 				{
// 					pattern.stone_pos[i] = -1;
// 					pattern.count = 1;
// 				}
// 				if  (i == 3)
// 					pattern.count = 3;
// 			}
// 			pattern.hole_pos[0] = start_pos + 1 * 21;
// 			pattern.hole_pos[1] = start_pos + 2 * 21;
// 			pattern.oposant_pos[0] = (y - 2 >= 0 && x - 2 >= 0) ? (start_pos - 2 * 21) : -1;
// 			pattern.oposant_pos[1] = (y - 1 >= 0 && x - 1 >= 0) ? (start_pos - 1 * 21) : -1;
// 			pattern.oposant_pos[2] = (y + 3 < 19 && x + 3 < 19) ? (start_pos + 3 * 21) : -1;
// 			pattern.oposant_pos[3] = (y + 4 < 19 && x + 4 < 19) ? (start_pos + 4 * 21) : -1;
// 			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_DIAG_G);
// 			total_patterns3++;
// 		}
// 	}

// 	// 4. Diagonale / (stride = 19)
// 	for (int y = 0; y < 17; y++)
// 	{
// 		for (int x = 2; x < 19; x++)
// 		{
// 			t_PatternGroupe3 pattern = {};
// 			int start_pos = y * 20 + x;
			
// 			for (int i = 0; i < 4; i++)
// 			{
// 				pattern.stone_pos[i] = start_pos + i * 19;
// 				if (y + i == 19)
// 				{
// 					pattern.stone_pos[i] = -1;
// 					pattern.count = 1;
// 				}
// 				if  (i == 3)
// 					pattern.count = 3;
// 			}
// 			pattern.hole_pos[0] = start_pos + 1 * 19;
// 			pattern.hole_pos[1] = start_pos + 2 * 19;
// 			pattern.oposant_pos[0] = (y - 2 >= 0 && x + 2 < 19) ? (start_pos - 2 * 19) : -1;
// 			pattern.oposant_pos[1] = (y - 1 >= 0 && x + 1 < 19) ? (start_pos - 1 * 19) : -1;
// 			pattern.oposant_pos[2] = (y + 3 < 19 && x - 3 >= 0) ? (start_pos + 3 * 19) : -1;
// 			pattern.oposant_pos[3] = (y + 4 < 19 && x - 4 >= 0) ? (start_pos + 4 * 19) : -1;
// 			add_pattern_group3_to_lookup(lookup_table3, &pattern, DIR_DIAG_D);
// 			total_patterns3++;
// 		}
// 	}
// 	std::cout << "total_patterns3: " << total_patterns3 << std::endl;
// }

// int	check_three_align(const t_PatternList_Groupe3 lookup_table[361][4], const bitboard19 &boardA, const bitboard19 &boardB,
// 						const int x, const int y)
// {
// 	const int	idx = IDX(x, y);
// 	int			total_score = 0;
// 	int			double_three = 0;

// 	for (int dir = 0; dir < 4; dir++) // parcour des 4 directions
// 	{
// 		const t_PatternList_Groupe3	&list_groupe = lookup_table[idx][dir];
// 		const int	count = list_groupe.count;

// 		for (int pos = 0; pos < count; pos++)
// 		{
// 			// cas 00xxx00					valeur max : vaut 8
// 			// 10xxx00, 00xx01				valeur moyen, same: oposant_pos[0] et [3] = 128
// 			// 10xxx01, 01xxx00	 00xxx10	valeur min, same: oposant_pos[1] et [2] = 256

// 			// 00X0XX0 et 00XX0X0			valeur max: vaut 12
// 			// 10X0XX0, 00X0XX1, 01XX0X0, 00XX0X1	valeur moyen
// 			// 10X0XX1 								valeur min

// 			// double three : on divise la valeur par 4;
// 			const t_PatternGroupe3& pattern = list_groupe.patterns[pos];
// 			int		score = 0;
// 			score  = 0;

// 			if (pattern.oposant_pos[1] == -1 || get_bb19(boardB, pattern.oposant_pos[1]))
// 				score = 256;
// 			else if (pattern.oposant_pos[0] == -1 || get_bb19(boardB, pattern.oposant_pos[0]))
// 				score = 128;
// 			if (pattern.oposant_pos[2] == -1 || get_bb19(boardB, pattern.oposant_pos[2]))
// 				score += 256;
// 			else if (pattern.oposant_pos[3] == -1 || get_bb19(boardB, pattern.oposant_pos[3]))
// 				score += 128;
			
// 			if (score > 256)
// 			{
// 					continue; // alignement de 3 pierre complètement fermé, on ignore
// 			}	
				
// 			if (get_bb19(boardA, pattern.stone_pos[0]) && 
// 				get_bb19(boardA, pattern.stone_pos[1]) &&
// 				get_bb19(boardA, pattern.stone_pos[2]))
// 			{
// 				// std::cout << "alignement de 3 pierre complètement ouvert, on ignore, score = " << score << std::endl;
// 				total_score += score + 8;
// 				double_three += 1;
// 				continue;
// 			}
// 			if (pattern.stone_pos[3] == -1)
// 				continue;
// 			// std::cout << "pattern: hole pos[0] = " << pattern.hole_pos[0] << "hole pos[1] = " << pattern.hole_pos[1] << ", stone pos = " << pattern.stone_pos[0] << std::endl;
// 			if (!get_bb19(boardB, pattern.hole_pos[0]) && get_bb19(boardA, pattern.stone_pos[0]) && get_bb19(boardA, pattern.stone_pos[2]) &&
// 				get_bb19(boardA, pattern.stone_pos[3]))
// 			{
// 				total_score += score + 12; // a change
// 				double_three += 1;
// 				// std::cout << "alignement de 3 pierre avec troue, on ignore, score = " << score << std::endl;
// 				continue;
// 			}
// 			if (!get_bb19(boardB, pattern.hole_pos[1]) && get_bb19(boardA, pattern.stone_pos[0]) && get_bb19(boardA, pattern.stone_pos[1]) &&
// 				get_bb19(boardA, pattern.stone_pos[3]))
// 			{
// 				total_score += score + 12;
// 				double_three += 1;
// 				// std::cout << "alignement de 3 pierre avec troue 2, on ignore, score = " << score << std::endl;
// 				continue;
// 			}
// 		}
// 		if (double_three == 2)
// 			return total_score / 4;
// 	}
// 	if (double_three == 0)
// 		return 0;
// 	return total_score;
// }


// static void	add_pattern_super4(t_PatternList_super4 lookup_table_super4[361][4], t_super4 *pattern, int start_x, int start_y, int dir)
// {
// 	for (int i = 0; i < 7; i++)
// 	{
// 		int case_x, case_y;
// 		switch (dir)
// 		{
// 			case DIR_HORIZ:
// 				case_x = start_x + i;
// 				case_y = start_y;
// 				break;
// 			case DIR_VERT:
// 				case_x = start_x;
// 				case_y = start_y + i;
// 				break;
// 			case DIR_DIAG_G:
// 				case_x = start_x - i;
// 				case_y = start_y + i;
// 				break;
// 			case DIR_DIAG_D:
// 				case_x = start_x + i;
// 				case_y = start_y + i;
// 				break;
// 		}
// 		// Sécurité : on reste dans le plateau
// 		if (case_x < 0 || case_x >= 19 || case_y < 0 || case_y >= 19)
// 			continue;

// 		int cell = IDX(case_x, case_y);
// 		t_PatternList_super4 *list = &lookup_table_super4[cell][dir];

// 		if (list->count < 7)
// 		{
// 			list->patterns[list->count] = *pattern; // copie du pattern
// 			list->count++;
// 		}
// 	}
// }


// void build_lookup_table_super4(t_PatternList_super4 lookup_table[361][4])
// {
// 	// horizontal
// 	for (int y = 0; y < 19; y++)
// 	{
// 		for (int x = 0; x < 13; x++)
// 		{
// 			t_super4	pattern = {};
// 			int			start_pos = y * 20 + x;

// 			for (int i = 0; i < 7; i++)
// 			{
// 				int	bit = start_pos + i;
// 				int	idx = bit >> 6;
// 				int	offset = bit & 63;

// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			clear_bit(pattern.mask, x + 1, y);
// 			pattern.hole_pos[0] = index_bb19(x + 1, y);
// 			clear_bit(pattern.mask, x + 5, y);
// 			pattern.hole_pos[1] = index_bb19(x + 5, y);
// 			add_pattern_super4(lookup_table, &pattern, x, y, DIR_HORIZ);
// 		}
// 	}

// 	// vertical
// 	for (int y = 0; y < 13; y++)
// 	{
// 		for (int x = 0; x < 19; x++)
// 		{
// 			t_super4	pattern = {};
// 			int			start_pos = y * 20 + x;

// 			for (int i = 0; i < 7; i++)
// 			{
// 				int	bit = start_pos + i * 20;
// 				int	idx = bit >> 6;
// 				int	offset = bit & 63;

// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			clear_bit(pattern.mask, x, y + 1);
// 			pattern.hole_pos[0] = index_bb19(x, y + 1);
// 			clear_bit(pattern.mask, x, y + 5);
// 			pattern.hole_pos[1] = index_bb19(x, y + 5);
// 			add_pattern_super4(lookup_table, &pattern, x, y, DIR_VERT);
// 		}
// 	}

// 	// diagonal \ (stride = 21)
// 	for (int y = 0; y < 13; y++)
// 	{
// 		for (int x = 0; x < 13; x++)
// 		{
// 			t_super4	pattern = {};
// 			int			start_pos = y * 20 + x;

// 			for (int i = 0; i < 7; i++)
// 			{
// 				int	bit = start_pos + i * 21;
// 				int	idx = bit >> 6;
// 				int	offset = bit & 63;

// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			clear_bit(pattern.mask, x + 1, y + 1);
// 			pattern.hole_pos[0] = index_bb19(x + 1, y + 1);
// 			clear_bit(pattern.mask, x + 5, y + 5);
// 			pattern.hole_pos[1] = index_bb19(x + 5, y + 5);
// 			add_pattern_super4(lookup_table, &pattern, x, y, DIR_DIAG_D);
// 		}
// 	}

// 	// diagonal / (stride = 19)
// 	for (int y = 0; y < 13; y++)
// 	{
// 		for (int x = 6; x < 19; x++)
// 		{
// 			t_super4	pattern = {};
// 			int			start_pos = y * 20 + x;

// 			for (int i = 0; i < 7; i++)
// 			{
// 				int	bit = start_pos + i * 19;
// 				int	idx = bit >> 6;
// 				int	offset = bit & 63;

// 				pattern.mask[idx] |= (1ULL << offset);
// 			}
// 			clear_bit(pattern.mask, x - 1, y + 1);
// 			pattern.hole_pos[0] = index_bb19(x - 1, y + 1);
// 			clear_bit(pattern.mask, x - 5, y + 5);
// 			pattern.hole_pos[1] = index_bb19(x - 5, y + 5);
// 			add_pattern_super4(lookup_table, &pattern, x, y, DIR_DIAG_G);
// 		}
// 	}
// }

// int	check_super4(const t_PatternList_super4 lookup_table[361][4], const bitboard19 &boardA, const bitboard19 &boardB,
// 						const int x, const int y)
// {
// 	const int	idx = IDX(x, y);

// 	for (int dir = 0; dir < 4; dir++)
// 	{
// 		const t_PatternList_super4	&list_super4 = lookup_table[idx][dir];
// 		int count = list_super4.count;

// 		for (int i = 0; i < count; i++)
// 		{
// 			const t_super4& pattern = list_super4.patterns[i];

// 			if (match_pattern(pattern.mask, boardA) &&
// 				!get_bb19(boardB, pattern.hole_pos[0]) &&
// 				!get_bb19(boardB, pattern.hole_pos[1]))
// 			{
// 				return 1; // super four détecté
// 			}
// 		}
// 	}
// 	return 0;
// }


// static void	add_pattern_cross_to_lookup(t_PatternList_Cross lookup_table[361], t_cross *pattern)
// {
// 	int positions[5] = {pattern->middle, pattern->up, pattern->down, pattern->left, pattern->right};

// 	for (int i = 0; i < 5; i++)
// 	{
// 		int pos = positions[i];

// 		int x = pos % 20;
// 		int y = pos / 20;
// 		int cell = IDX(x, y);

// 		t_PatternList_Cross *list = &lookup_table[cell];
// 		if (list->count < 5)
// 		{
// 			list->cross[list->count] = *pattern;
// 			list->count++;
// 		}
// 	}
// }

// void	build_lookup_table_cross(t_PatternList_Cross lookup_table[361])
// {
// 	for (int y = 1; y < 18; y++)
// 	{
// 		for (int x = 1; x < 18; x++)
// 		{
// 			t_cross	pattern = {};
// 			int		start_pos = y * 20 + x; // position de la pierre centrale du cross

// 			pattern.middle = start_pos;
// 			pattern.up = start_pos - 20;
// 			pattern.down = start_pos + 20;
// 			pattern.left = start_pos - 1;
// 			pattern.right = start_pos + 1;

// 			pattern.opposant_up[0] = (y - 3 >= 0) ? (start_pos - 3 * 20) : -1;
// 			pattern.opposant_up[1] = (y - 2 >= 0) ? (start_pos - 2 * 20) : -1;

// 			pattern.opposant_down[0] = (y + 3 < 19) ? (start_pos + 3 * 20) : -1;
// 			pattern.opposant_down[1] = (y + 2 < 19) ? (start_pos + 2 * 20) : -1;

// 			pattern.opposant_left[0] = (x - 3 >= 0) ? (start_pos - 3) : -1;
// 			pattern.opposant_left[1] = (x - 2 >= 0) ? (start_pos - 2) : -1;

// 			pattern.opposant_right[0] = (x + 3 < 19) ? (start_pos + 3) : -1;
// 			pattern.opposant_right[1] = (x + 2 < 19) ? (start_pos + 2) : -1;
// 			add_pattern_cross_to_lookup(lookup_table, &pattern);
// 		}
// 	}
// }

// int	check_cross(const t_PatternList_Cross lookup_table[361], const bitboard19 &boardA, const bitboard19 &boardB
// 					, const int x, const int y)
// {
// 	const int	idx = IDX(x, y);
// 	const	t_PatternList_Cross	&list_cross = lookup_table[idx];

// 	for (int p = 0; p < list_cross.count; p++)
// 	{
// 		const t_cross	&cross = list_cross.cross[p];
// 		int			opposant_score = 0;
// 		int			score = 0;

// 		if (get_bb19(boardB, cross.middle) ||
// 			get_bb19(boardB, cross.up) || get_bb19(boardB, cross.down) ||
// 			get_bb19(boardB, cross.left) || get_bb19(boardB, cross.right))
// 			continue;

// 		if (cross.opposant_up[1] == -1 || get_bb19(boardB, cross.opposant_up[1]))
// 			opposant_score = 64;
// 		else if (cross.opposant_up[0] == -1 || get_bb19(boardB, cross.opposant_up[0]))
// 			opposant_score = 32;
// 		if (cross.opposant_down[1] == -1 || get_bb19(boardB, cross.opposant_down[1]))
// 			opposant_score += 64;
// 		else if (cross.opposant_down[0] == -1 || get_bb19(boardB, cross.opposant_down[0]))
// 			opposant_score += 32;
// 		if (cross.opposant_left[1] == -1 || get_bb19(boardB, cross.opposant_left[1]))
// 			opposant_score += 64;
// 		else if (cross.opposant_left[0] == -1 || get_bb19(boardB, cross.opposant_left[0]))
// 			opposant_score += 32;
// 		if (cross.opposant_right[1] == -1 || get_bb19(boardB, cross.opposant_right[1]))
// 			opposant_score += 64;
// 		else if (cross.opposant_right[0] == -1 || get_bb19(boardB, cross.opposant_right[0]))
// 			opposant_score += 32;

// 		if (opposant_score > 64)
// 			continue;

// 		if (get_bb19(boardA, cross.middle))
// 			score += 3;
// 		if (get_bb19(boardA, cross.up))
// 			score += 4;
// 		if (get_bb19(boardA, cross.down))
// 			score += 4;
// 		if (get_bb19(boardA, cross.left))
// 			score += 4;
// 		if (get_bb19(boardA, cross.right))
// 			score += 4;
		
// 		if (score >= 15) // on a au moins une croix partiel
// 			return score + opposant_score;
// 	}
// 	return 0;
// }

// void	test_bitboard(const GameBoard& board, int x, int y)
// {
// 	static t_PatternList5	lookup_table5[361][4] = {};  // 19*19 = 361 positions possibles, 4 directions
// 	static t_PatternList4	lookup_table4[361][4] = {};
// 	static t_PatternList_Groupe4 lookup_table_groupe4[361][4] = {};
// 	static t_PatternList_Groupe3 lookup_table3[361][4] = {};
// 	static t_PatternList_super4 lookup_table_super4[361][4] = {};
// 	static t_PatternList_Cross lookup_table_cross[361] = {};
// 	static bool initialized = false;

// 	if (!initialized)
// 	{
// 		build_lookup_table5(lookup_table5);
// 		build_lookup_table4(lookup_table4);
// 		build_lookup_table_groupe4(lookup_table_groupe4);
// 		build_lookup_table3(lookup_table3);
// 		build_lookup_table_super4(lookup_table_super4);
// 		build_lookup_table_cross(lookup_table_cross);
// 		initialized = true;
// 	}

// 	t_BWBoard19	bitboard = GameBoard_to_bitboard(board);
// 	print_bb_19(bitboard);

// 	std::cout << "test bitboard : x = " << x << " y = " << y << std::endl;


// 	if (isWin_ultra(lookup_table5, bitboard.black, x, y))
// 		std::cout << "les noires ont gagne !!!" << std::endl;
// 	if (isWin_ultra(lookup_table5, bitboard.white, x, y))
// 		std::cout << "les blanches ont gagne !!!" << std::endl;

// 	int	black_four = is_Open_4(lookup_table4, bitboard.black, bitboard.white, x, y);
// 	int white_four = is_Open_4(lookup_table4, bitboard.white, bitboard.black, x, y);
// 	if (black_four == 1)
// 		std::cout << "alignement de 4 pierre noir partiellement ouvert" << std::endl;
// 	else if (black_four == 2)
// 		std::cout << "Open four black !!!" << std::endl;
// 	if (white_four == 1)
// 		std::cout << "alignement de 4 pierre blanche partiellement ouvert" << std::endl;
// 	else if (white_four == 2)
// 		std::cout << "Open four white !!!" << std::endl;

// 	if (check_four_align(lookup_table_groupe4, bitboard.black, bitboard.white, x, y) > 0)
// 		std::cout << "alignement de 4 pierre noir avec troue" << std::endl;
// 	else if (check_four_align(lookup_table_groupe4, bitboard.white, bitboard.black, x, y) > 0)
// 		std::cout << "alignement de 4 pierre blanche avec troue" << std::endl;
	
// 	int	three_black = check_three_align(lookup_table3, bitboard.black, bitboard.white, x, y);
// 	int	three_white = check_three_align(lookup_table3, bitboard.white, bitboard.black, x, y);
// 	if (three_black > 0)
// 		std::cout << "valeur de retour, detection three black:  " << three_black << std::endl;
// 	if (three_white > 0)
// 		std::cout << "valeur de retour, detection three white:  " << three_white << std::endl;

// 	if (check_super4(lookup_table_super4, bitboard.black, bitboard.white, x, y))
// 		std::cout << "super four noir detecte !!!" << std::endl;
// 	if (check_super4(lookup_table_super4, bitboard.white, bitboard.black, x, y))
// 		std::cout << "super four blanc detecte !!!" << std::endl;

// 	int cross_black = check_cross(lookup_table_cross, bitboard.black, bitboard.white, x, y);
// 	int cross_white = check_cross(lookup_table_cross, bitboard.white, bitboard.black, x, y);
// 	if (cross_black > 0)
// 		std::cout << "croix noir detecte, score = " << cross_black << std::endl;
// 	if (cross_white > 0)
// 		std::cout << "croix blanche detecte, score = " << cross_white << std::endl;

// }
