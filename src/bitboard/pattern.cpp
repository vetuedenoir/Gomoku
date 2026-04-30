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

// void add_groupe_mask_to_lookup4(t_MaskList_Groupe4 lookup_table_groupe4[361][4], bitboard19 all_masks4[MAX_FOUR_MASK],
// 	const int mask_index, const int start_pos, const int stride)
// {
// 	// int dir;
// }

void add_pattern_to_lookup(t_MaskList_Groupe4 lookup_table[361][4], t_PatternGroup *gmask, 
                           int start_x, int start_y, int dir, int hole_index)
{
    // Pour chaque case du pattern (5 cases)
    for (int i = 0; i < 5; i++) {
        // Calcule la position de la case
        int case_x, case_y = 0;
        
        switch (dir) {
            case DIR_HORIZ: // Horizontal
                case_x = start_x + i;
                case_y = start_y;
                break;
            case DIR_VERT: // Vertical
                case_x = start_x;
                case_y = start_y + i;
                break;
				case DIR_DIAG_G: // Diagonale /
                case_x = start_x - i;
                case_y = start_y + i;
                break;
			case DIR_DIAG_D: // Diagonale
				case_x = start_x + i;
				case_y = start_y + i;
				break;
			default:
				return; // Direction invalide
        }
        
        // Vérifie si cette case est le trou (à sauter)
        int current_index = index_bb19(case_x, case_y);
        if (current_index == hole_index) {
            continue;  // Saute le trou
        }
        
        // Ajoute le pattern à la lookup table pour cette case et direction
        int cell = case_y * 19 + case_x;
        t_MaskList_Groupe4 *list = &lookup_table[cell][dir];
        
        // Copie le pattern
        list->masks[list->count] = *gmask;
        list->count++;
    }
}

// void build_lookup_table_groupe4(t_MaskList_Groupe4 lookup_table_groupe4[361][4], t_PatternGroup all_masks4[MAX_FOUR_MASK])
// {
// 	int total_masks4 = 0;

// 	lookup_table_groupe4[0][0].count = 0;

// 	// 1. Horizontal (stride = 1)
// 	for (int y = 0; y < 19; y++) {
// 		for (int x = 0; x <= 14; x++) {
// 			t_PatternGroup gmask = {};
// 			t_BWBoard19 board1 = {};
// 			t_BWBoard19 board2 = {};
// 			t_BWBoard19 board3 = {};

// 			int start_pos = y * 20 + x;
// 			for (int i = 0; i < 5; i++) {
// 				int bit = start_pos + i;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.patterns[0][idx] |= (1ULL << offset);
// 				gmask.patterns[1][idx] |= (1ULL << offset);
// 				gmask.patterns[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 2; i++)
// 			{
// 				clear_bit(gmask.patterns[i], x + 1 + i, y );
// 				gmask.middle_pos[i] = index_bb19(x + 1 + i, y);
// 			}
// 			all_masks4[total_masks4] = gmask;
// 			add_pattern_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_HORIZ);


// 			memcpy(board1.white, gmask.patterns[0], sizeof(board1.white));
// 			memcpy(board2.white, gmask.patterns[1], sizeof(board2.white));
// 			memcpy(board3.white, gmask.patterns[2], sizeof(board3.white));
			
// 			print_bb_19_colored(board1);
// 			std::cout << "index of middle pos: " << index_bb19(x, y) << std::endl;
// 			std::cout << "start pos: " << start_pos << std::endl;
// 			print_bb_19_colored(board2);
// 			std::cout << "index of middle pos: " << index_bb19(x, y + 2) << std::endl;
// 			print_bb_19_colored(board3);
// 			std::cout << "index of middle pos: " << index_bb19(x, y + 3) << std::endl;
// 			total_masks4++;
// 		}
// 	}
	
// 	// 2. Vertical (stride = 20)
// 	for (int x = 0; x < 19; x++) {
// 		for (int y = 0; y <= 14; y++) {
// 			t_PatternGroup gmask = {};
// 			int start_pos = y * 20 + x;
// 			for (int i = 0; i < 5; i++) {
// 				int bit = start_pos + i * 20;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.patterns[0][idx] |= (1ULL << offset);
// 				gmask.patterns[1][idx] |= (1ULL << offset);
// 				gmask.patterns[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 2; i++)
// 			{
// 				clear_bit(gmask.patterns[i], x, y + 1 + i);
// 				gmask.middle_pos[i] = index_bb19(x, y + 1 + i);
// 			}
// 			all_masks4[total_masks4] = gmask;
// 			add_pattern_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_VERT);
// 			total_masks4++;
// 		}
// 	}
	
// 	// // 3. Diagonale \ (stride = 21)
// 	for (int y = 0; y <= 14; y++) {
// 		for (int x = 0; x <= 14; x++) {
// 			t_PatternGroup gmask = {};
// 			int start_pos = y * 20 + x;
// 			for (int i = 0; i < 5; i++) {
// 				int bit = start_pos + i * 21;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.patterns[0][idx] |= (1ULL << offset);
// 				gmask.patterns[1][idx] |= (1ULL << offset);
// 				gmask.patterns[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 2; i++)
// 			{
// 				clear_bit(gmask.patterns[i], x + 1 + i, y + 1 + i);
// 				gmask.middle_pos[i] = index_bb19(x + 1 + i, y + 1 + i);
// 			}
// 			all_masks4[total_masks4] = gmask;
// 			add_pattern_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_D);
// 			total_masks4++;
// 		}
// 	}
	
// 	// // 4. Diagonale / (stride = 19)
// 	for (int y = 0; y <= 14; y++) {
// 		for (int x = 4; x < 19; x++) {
// 			 t_PatternGroup gmask = {};
// 			int start_pos = y * 20 + x;
// 			for (int i = 0; i < 5; i++) {
// 				int bit = start_pos + i * 19;
// 				int idx = bit >> 6;
// 				int offset = bit & 63;
// 				gmask.patterns[0][idx] |= (1ULL << offset);
// 				gmask.patterns[1][idx] |= (1ULL << offset);
// 				gmask.patterns[2][idx] |= (1ULL << offset);
// 			}
// 			for (int i = 0; i < 2; i++)
// 			{
// 				clear_bit(gmask.patterns[i], x - 1 - i, y + 1 + i);
// 				gmask.middle_pos[i] = index_bb19(x - 1 - i, y + 1 + i);
// 			}
// 			all_masks4[total_masks4] = gmask;
// 			add_pattern_to_lookup(lookup_table_groupe4, &gmask, x, y, DIR_DIAG_G);
// 			total_masks4++;
// 		}
// 	}
// 	std::cout << "total_masks4: " << total_masks4 << std::endl;
// }

void build_lookup_table_groupe4(t_MaskList_Groupe4 lookup_table[361][4])
{
    // 1. Horizontal (stride = 1)
    for (int y = 0; y < 19; y++) {
        for (int x = 0; x <= 14; x++) {
            for (int p = 0; p < 3; p++) {
                t_PatternGroup gmask = {};
                int start_pos = y * 20 + x;
                int hole_index = 0;
                
                // Remplit les 5 cases
                for (int i = 0; i < 5; i++) {
                    int bit = start_pos + i;
                    int idx = bit >> 6;
                    int offset = bit & 63;
                    gmask.patterns[p][idx] |= (1ULL << offset);
                }
                
                // Efface une case différente et enregistre hole_index
                if (p == 0) {
                    hole_index = index_bb19(x + 1, y);
                    clear_bit(gmask.patterns[0], x + 1, y);
                    gmask.hole_pos[p] = hole_index;
                } else if (p == 1) {
                    hole_index = index_bb19(x + 2, y);
                    clear_bit(gmask.patterns[1], x + 2, y);
                    gmask.hole_pos[p] = hole_index;
                } else {
                    hole_index = index_bb19(x + 3, y);
                    clear_bit(gmask.patterns[2], x + 3, y);
                    gmask.hole_pos[p] = hole_index;
                }
                
                // Ajoute à la lookup table
                add_pattern_to_lookup(lookup_table, &gmask, x, y, DIR_HORIZ, hole_index);
            }
        }
    }
    
    // 2. Vertical (stride = 20)
    for (int x = 0; x < 19; x++) {
        for (int y = 0; y <= 14; y++) {
            for (int p = 0; p < 3; p++) {
                t_PatternGroup gmask = {};
                int start_pos = y * 20 + x;
                int hole_index = 0;
                
                for (int i = 0; i < 5; i++) {
                    int bit = start_pos + i * 20;
                    int idx = bit >> 6;
                    int offset = bit & 63;
                    gmask.patterns[p][idx] |= (1ULL << offset);
                }
                
                if (p == 0) {
                    hole_index = index_bb19(x, y + 1);
                    clear_bit(gmask.patterns[0], x, y + 1);
                    gmask.hole_pos[p] = hole_index;
                } else if (p == 1) {
                    hole_index = index_bb19(x, y + 2);
                    clear_bit(gmask.patterns[1], x, y + 2);
                    gmask.hole_pos[p] = hole_index;
                } else {
                    hole_index = index_bb19(x, y + 3);
                    clear_bit(gmask.patterns[2], x, y + 3);
                    gmask.hole_pos[p] = hole_index;
                }
                
                add_pattern_to_lookup(lookup_table, &gmask, x, y, DIR_VERT, hole_index);
            }
        }
    }
    
    // 3. Diagonale \ (stride = 21)
    for (int y = 0; y <= 14; y++) {
        for (int x = 0; x <= 14; x++) {
            for (int p = 0; p < 3; p++) {
                t_PatternGroup gmask = {};
                int start_pos = y * 20 + x;
                int hole_index = 0;
                
                for (int i = 0; i < 5; i++) {
                    int bit = start_pos + i * 21;
                    int idx = bit >> 6;
                    int offset = bit & 63;
                    gmask.patterns[p][idx] |= (1ULL << offset);
                }
                
                if (p == 0) {
                    hole_index = index_bb19(x + 1, y + 1);
                    clear_bit(gmask.patterns[0], x + 1, y + 1);
                    gmask.hole_pos[p] = hole_index;
                } else if (p == 1) {
                    hole_index = index_bb19(x + 2, y + 2);
                    clear_bit(gmask.patterns[1], x + 2, y + 2);
                    gmask.hole_pos[p] = hole_index;
                } else {
                    hole_index = index_bb19(x + 3, y + 3);
                    clear_bit(gmask.patterns[2], x + 3, y + 3);
                    gmask.hole_pos[p] = hole_index;
                }
                
                add_pattern_to_lookup(lookup_table, &gmask, x, y, DIR_DIAG_D, hole_index);
            }
        }
    }
    
    // 4. Diagonale / (stride = 19)
    for (int y = 0; y <= 14; y++) {
        for (int x = 4; x < 19; x++) {
            for (int p = 0; p < 3; p++) {
                t_PatternGroup gmask = {};
                int start_pos = y * 20 + x;
                int hole_index = 0;
                
                for (int i = 0; i < 5; i++) {
                    int bit = start_pos + i * 19;
                    int idx = bit >> 6;
                    int offset = bit & 63;
                    gmask.patterns[p][idx] |= (1ULL << offset);
                }
                
                if (p == 0) {
                    hole_index = index_bb19(x - 1, y + 1);
                    clear_bit(gmask.patterns[0], x - 1, y + 1);
                    gmask.hole_pos[p] = hole_index;
                } else if (p == 1) {
                    hole_index = index_bb19(x - 2, y + 2);
                    clear_bit(gmask.patterns[1], x - 2, y + 2);
                    gmask.hole_pos[p] = hole_index;
                } else {
                    hole_index = index_bb19(x - 3, y + 3);
                    clear_bit(gmask.patterns[2], x - 3, y + 3);
                    gmask.hole_pos[p] = hole_index;
                }
                
                add_pattern_to_lookup(lookup_table, &gmask, x, y, DIR_DIAG_G, hole_index);
            }
        }
    }
}



void	test_bitboard(const GameBoard& board, int x, int y)
{
	// static bitboard19	all_masks5[MAX_WINNING_MASK] = {};
	// static t_MaskList5	lookup_table5[361][4] = {};  // 19*19 = 361 positions possibles, 4 directions

	// static bitboard19	all_masks4[MAX_FOUR_MASK] = {};
	// static t_MaskList4	lookup_table4[361][4] = {};

	static t_MaskList_Groupe4 lookup_table_groupe4[361][4] = {};

	static bool initialized = false;
	if (!initialized) {
		// build_lookup_table5(lookup_table5, all_masks5);
		// build_lookup_table4(lookup_table4, all_masks4);
		build_lookup_table_groupe4(lookup_table_groupe4);
		initialized = true;
	}

	t_BWBoard19	bitboard = GameBoard_to_bitboard(board);
	print_bb_19(bitboard);

	std::cout << "test bitboard : x = " << x << " y = " << y << std::endl;

	// // build_lookup_table5(lookup_table5, all_masks5);
	// // build_lookup_table4(lookup_table4, all_masks4);

	// if (isWin_ultra(lookup_table5, bitboard.black, x, y))
	// 	std::cout << "les noires ont gagne !!!" << std::endl;
	// if (isWin_ultra(lookup_table5, bitboard.white, x, y))
	// 	std::cout << "les blanches ont gagne !!!" << std::endl;			// for (int w = 0; w < 6; w++) {
			// 	all_masks4[total_masks4][w] = mask[w];
			// }
			// add_groupe_mask_to_lookup4(lookup_table_groupe4, all_masks4, total_masks4, start_pos, 19);

	// int	black_four = is_Open_4(lookup_table4, bitboard.black, bitboard.white, x, y);
	// int white_four = is_Open_4(lookup_table4, bitboard.white, bitboard.black, x, y);
	// if (black_four == 1)
	// 	std::cout << "alignement de 4 pierre noir partiellement ouvert" << std::endl;
	// else if (black_four == 2)
	// 	std::cout << "Open four black !!!" << std::endl;
	// if (white_four == 1)
	// 	std::cout << "alignement de 4 pierre blanche partiellement ouvert" << std::endl;
	// else if (white_four == 2)
	// 	std::cout << "Open four white !!!" << std::endl;
}
