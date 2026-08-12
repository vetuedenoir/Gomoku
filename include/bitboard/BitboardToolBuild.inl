
template<typename Traits>
void BitboardTool<Traits>::buildAll()
{
	build_lookup_table5();
	build_lookup_table4();
	build_lookup_table_groupe4();
	build_lookup_table3();
	build_lookup_table_super4();
	build_lookup_table_cross();
}

template<typename Traits>
void BitboardTool<Traits>::add_mask_to_lookup5(const typename Traits::Bitboard& mask, const int start_pos, const int stride)
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
		
		t_PatternList5<Traits> *list = &_lt5[idx][dir];
		const uint32_t slot = list->count;
		
		for (int w = 0; w < Traits::WORD_COUNT; w++)
			list->masks[slot][w] = mask[w];
		computeMaskSpan(mask, list->firstWord[slot], list->lastWord[slot]);
		list->count++;
	}
}

template<typename Traits>
void BitboardTool<Traits>::build_lookup_table5()
{

	memset(_lt5, 0, sizeof(t_PatternList5<Traits>) * Traits::CELL_COUNT * 4);
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
			add_mask_to_lookup5( mask, start_pos, 1);
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
			add_mask_to_lookup5( mask, start_pos, Traits::STRIDE);
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
			add_mask_to_lookup5( mask, start_pos, Traits::STRIDE_D);
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
			add_mask_to_lookup5( mask, start_pos, Traits::STRIDE_G);
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::add_mask_to_lookup4(t_Pattern4<Traits>* pattern, const int start_pos, const int stride)
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

	computeMaskSpan(pattern->mask, pattern->firstWord, pattern->lastWord);

	for (int i = 0; i < 4; i++)
	{
		int pos = start_pos + i * stride;
		int y = pos / Traits::STRIDE;
		int x = pos % Traits::STRIDE;
		int idx = idx_generic<Traits>(x, y);
		
		t_PatternList4<Traits> *list = &_lt4[idx][dir];
		
		list->patterns[list->count] = *pattern;
		list->count++;
	}
}

template<typename Traits>
void BitboardTool<Traits>::build_lookup_table4()
{
	memset(_lt4, 0, sizeof(t_PatternList4<Traits>) * Traits::CELL_COUNT * 4);
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

			add_mask_to_lookup4( &pattern, start_pos, 1);
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
			add_mask_to_lookup4( &pattern, start_pos, Traits::STRIDE);
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
			add_mask_to_lookup4( &pattern, start_pos, Traits::STRIDE_D);
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
			add_mask_to_lookup4( &pattern, start_pos, Traits::BOARD_SIZE);
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::add_pattern_group4(t_PatternGroup4<Traits>* group, int start_x, int start_y, int dir)
{
	for (int m = 0; m < 3; ++m)
		computeMaskSpan(group->masks[m], group->firstWord[m], group->lastWord[m]);

    for (int i = 0; i < 5; i++)
	{
        int case_x = 0;
		int case_y = 0;

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
        t_PatternList_Groupe4<Traits> *list = &_ltg4[cell][dir];

        list->patterns[list->count] = *group;
        list->count++;
    }
}

template<typename Traits>
void BitboardTool<Traits>::build_lookup_table_groupe4()
{
	memset(_ltg4, 0, sizeof(t_PatternList_Groupe4<Traits>) * Traits::CELL_COUNT * 4);
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
			add_pattern_group4(&gmask, x, y, DIR_HORIZ);
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
			add_pattern_group4(&gmask, x, y, DIR_VERT);
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
			add_pattern_group4(&gmask, x, y, DIR_DIAG_D);
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
			add_pattern_group4(&gmask, x, y, DIR_DIAG_G);
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::finalize_group3_masks(t_PatternGroupe3<Traits>& pattern)
{
	pattern.fullMask = {};
	pattern.holeMask0 = {};
	pattern.holeMask1 = {};

	// SCORE_3_FULL : pierres en [0],[1],[2]
	set_bb_flate<Traits>(pattern.fullMask, pattern.stone_pos[0]);
	set_bb_flate<Traits>(pattern.fullMask, pattern.stone_pos[1]);
	set_bb_flate<Traits>(pattern.fullMask, pattern.stone_pos[2]);
	computeMaskSpan(pattern.fullMask, pattern.fullFirst, pattern.fullLast);

	// SCORE_3_HOLE : [0],[2],[3] (trou en [1]) et [0],[1],[3] (trou en [2])
	set_bb_flate<Traits>(pattern.holeMask0, pattern.stone_pos[0]);
	set_bb_flate<Traits>(pattern.holeMask0, pattern.stone_pos[2]);
	set_bb_flate<Traits>(pattern.holeMask0, pattern.stone_pos[3]);
	computeMaskSpan(pattern.holeMask0, pattern.hole0First, pattern.hole0Last);

	set_bb_flate<Traits>(pattern.holeMask1, pattern.stone_pos[0]);
	set_bb_flate<Traits>(pattern.holeMask1, pattern.stone_pos[1]);
	set_bb_flate<Traits>(pattern.holeMask1, pattern.stone_pos[3]);
	computeMaskSpan(pattern.holeMask1, pattern.hole1First, pattern.hole1Last);
}

template<typename Traits>
void BitboardTool<Traits>::add_pattern_group3_to_lookup(t_PatternGroupe3<Traits>* group, int dir)
{
	finalize_group3_masks(*group);

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
        
        t_PatternList_Groupe3<Traits> *list = &_lt3[cell][dir];
        if (list->count < 4)
		{
            list->patterns[list->count] = *group; // copie du pattern
            list->count++;
        }
    }

}

template<typename Traits>
void BitboardTool<Traits>::build_lookup_table3()
{
	memset(_lt3, 0, sizeof(t_PatternList_Groupe3<Traits>) * Traits::CELL_COUNT * 4);

	// 1. Horizontal (stride = 1)
	for (int y = 0; y < Traits::BOARD_SIZE; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 2; x++)
		{
			t_PatternGroupe3<Traits> pattern = {};
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
			add_pattern_group3_to_lookup( &pattern, DIR_HORIZ);
		}
	}

	// 2. Vertical (stride = Traits::STRIDE)
	for (int y = 0; y < Traits::BOARD_SIZE - 2; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE; x++)
		{
			t_PatternGroupe3<Traits> pattern = {};
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
			add_pattern_group3_to_lookup( &pattern, DIR_VERT);
		}
	}

	// 3. Diagonale \ (stride = 21)
	for (int y = 0; y < Traits::BOARD_SIZE - 2; y++)
	{
		for (int x = 0; x < Traits::BOARD_SIZE - 2; x++)
		{
			t_PatternGroupe3<Traits> pattern = {};
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
			add_pattern_group3_to_lookup( &pattern, DIR_DIAG_G);
		}
	}

	// 4. Diagonale / (stride = Traits::BOARD_SIZE)
	for (int y = 0; y < Traits::BOARD_SIZE - 2; y++)
	{
		for (int x = 2; x < Traits::BOARD_SIZE; x++)
		{
			t_PatternGroupe3<Traits> pattern = {};
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
			add_pattern_group3_to_lookup( &pattern, DIR_DIAG_D);
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::add_pattern_super4(t_super4<Traits>* pattern, int start_x, int start_y, int dir)
{
	computeMaskSpan(pattern->mask, pattern->firstWord, pattern->lastWord);

	for (int i = 0; i < 7; i++)
	{
		int case_x = 0;
		int case_y = 0;

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
		t_PatternList_super4<Traits> *list = &_lts4[cell][dir];

		if (list->count < 7)
		{
			list->patterns[list->count] = *pattern; // copie du pattern
			list->count++;
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::build_lookup_table_super4()
{
	memset(_lts4, 0, sizeof(t_PatternList_super4<Traits>) * Traits::CELL_COUNT * 4);
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
			add_pattern_super4( &pattern, x, y, DIR_HORIZ);
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
			add_pattern_super4( &pattern, x, y, DIR_VERT);
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
			add_pattern_super4( &pattern, x, y, DIR_DIAG_D);
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
			add_pattern_super4( &pattern, x, y, DIR_DIAG_G);
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::add_pattern_cross_to_lookup(t_cross* pattern)
{
	int positions[5] = {pattern->middle, pattern->up, pattern->down, pattern->left, pattern->right};

	for (int i = 0; i < 5; i++)
	{
		int pos = positions[i];

		int x = pos % Traits::STRIDE;
		int y = pos / Traits::STRIDE;
		int cell = idx_generic<Traits>(x, y);

		t_PatternList_Cross *list = &_ltcross[cell];
		if (list->count < 5)
		{
			list->cross[list->count] = *pattern;
			list->count++;
		}
	}
}

template<typename Traits>
void BitboardTool<Traits>::build_lookup_table_cross()
{
	memset(_ltcross, 0, sizeof(t_PatternList_Cross) * Traits::CELL_COUNT);

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
			add_pattern_cross_to_lookup( &pattern);
		}
	}
}