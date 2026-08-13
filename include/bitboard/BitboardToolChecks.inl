// BitboardTool pattern checks (included from BitboardTool.hpp)

template<typename Traits>
bool BitboardTool<Traits>::is_five_in_a_row(const typename Traits::Bitboard& stones,
                                            int col, int row) const
{
	return is_five_in_a_row_impl(stones, col, row, kPatternPrefilter);
}

template<typename Traits>
bool BitboardTool<Traits>::is_five_in_a_row_reference(const typename Traits::Bitboard& stones,
                                                      int col, int row) const
{
	return is_five_in_a_row_impl(stones, col, row, /*prefilter=*/false);
}

template<typename Traits>
bool BitboardTool<Traits>::is_five_in_a_row_impl(const typename Traits::Bitboard& stones,
                                                 int col, int row, bool prefilter) const
{
	const int idx = idx_generic<Traits>(col, row);

	for (int dir = 0; dir < 4; dir++)
	{
		if (prefilter && popWindow(_window[idx][dir], stones) < 5)
			continue;

		const t_PatternList5<Traits>& list = _lt5[idx][dir];

		for (uint32_t i = 0; i < list.count; i++)
		{
			if (matchPattern(list.masks[i], stones))
				return true;
		}
	}
	return false;
}

template<typename Traits>
int BitboardTool<Traits>::check_open_four(const typename Traits::Bitboard& stones,
                                          const typename Traits::Bitboard& opponent,
                                          int col, int row) const
{
	return check_open_four_impl(stones, opponent, col, row, kPatternPrefilter);
}

template<typename Traits>
int BitboardTool<Traits>::check_open_four_reference(const typename Traits::Bitboard& stones,
                                                    const typename Traits::Bitboard& opponent,
                                                    int col, int row) const
{
	return check_open_four_impl(stones, opponent, col, row, /*prefilter=*/false);
}

template<typename Traits>
int BitboardTool<Traits>::check_open_four_impl(const typename Traits::Bitboard& stones,
                                               const typename Traits::Bitboard& opponent,
                                               int col, int row, bool prefilter) const
{
	const int idx = idx_generic<Traits>(col, row);
	int opening_score = 2;

	for (int dir = 0; dir < 4; dir++)
	{
		if (prefilter && popWindow(_window[idx][dir], stones) < 4)
			continue;

		const t_PatternList4<Traits>& list = _lt4[idx][dir];
		const int count = list.count;

		for (int i = 0; i < count; i++)
		{
			const t_Pattern4<Traits>& pattern = list.patterns[i];

			if (!matchPattern(pattern.mask, stones))
				continue;
			if (pattern.opposant_left == -1
			    || get_bb_flate<Traits>(opponent, pattern.opposant_left))
				opening_score -= 1;
			if (pattern.opposant_right == -1
			    || get_bb_flate<Traits>(opponent, pattern.opposant_right))
				opening_score -= 1;
			return opening_score;
		}
	}
	return 0;
}

template<typename Traits>
int BitboardTool<Traits>::check_broken_four(const typename Traits::Bitboard& stones,
                                            const typename Traits::Bitboard& opponent,
                                            int col, int row) const
{
	return check_broken_four_impl(stones, opponent, col, row, kPatternPrefilter);
}

template<typename Traits>
int BitboardTool<Traits>::check_broken_four_reference(const typename Traits::Bitboard& stones,
                                                      const typename Traits::Bitboard& opponent,
                                                      int col, int row) const
{
	return check_broken_four_impl(stones, opponent, col, row, /*prefilter=*/false);
}

template<typename Traits>
int BitboardTool<Traits>::check_broken_four_impl(const typename Traits::Bitboard& stones,
                                                 const typename Traits::Bitboard& opponent,
                                                 int col, int row, bool prefilter) const
{
	const int idx = idx_generic<Traits>(col, row);

	for (int dir = 0; dir < 4; dir++)
	{
		if (prefilter && popWindow(_window[idx][dir], stones) < 4)
			continue;

		const t_PatternList_Groupe4<Traits>& list_groupe = _ltg4[idx][dir];
		const int count = list_groupe.count;

		for (int i = 0; i < count; i++)
		{
			const t_PatternGroup4<Traits>& groupe = list_groupe.patterns[i];

			if (!get_bb_flate<Traits>(opponent, groupe.hole_pos[0])
			    && matchPattern(groupe.masks[0], stones))
				return 1;
			if (!get_bb_flate<Traits>(opponent, groupe.hole_pos[1])
			    && matchPattern(groupe.masks[1], stones))
				return 2;
			if (!get_bb_flate<Traits>(opponent, groupe.hole_pos[2])
			    && matchPattern(groupe.masks[2], stones))
				return 3;
		}
	}
	return 0;
}

template<typename Traits>
int BitboardTool<Traits>::check_open_three(const typename Traits::Bitboard& stones,
                                           const typename Traits::Bitboard& opponent,
                                           int col, int row) const
{
	return check_open_three_impl(stones, opponent, col, row, kPatternPrefilter);
}

template<typename Traits>
int BitboardTool<Traits>::check_open_three_reference(const typename Traits::Bitboard& stones,
                                                     const typename Traits::Bitboard& opponent,
                                                     int col, int row) const
{
	return check_open_three_impl(stones, opponent, col, row, /*prefilter=*/false);
}

template<typename Traits>
int BitboardTool<Traits>::check_open_three_filtered(const typename Traits::Bitboard& stones,
                                                    const typename Traits::Bitboard& opponent,
                                                    int col, int row) const
{
	return check_open_three_impl(stones, opponent, col, row, /*prefilter=*/true);
}

template<typename Traits>
int BitboardTool<Traits>::check_open_three_impl(const typename Traits::Bitboard& stones,
                                                const typename Traits::Bitboard& opponent,
                                                int col, int row, bool prefilter) const
{
	const int idx = idx_generic<Traits>(col, row);
	int total_score = 0;
	int double_three = 0;

	for (int dir = 0; dir < 4; dir++)
	{
		// Lot-2 pre-filter: < 3 own stones in the ±4 window ⇒ no open-three
		// possible on this axis (query stone is already in `stones`).
		if (prefilter && popWindow(_window[idx][dir], stones) < 3)
			continue;

		const t_PatternList_Groupe3<Traits>& list_groupe = _lt3[idx][dir];
		const int count = list_groupe.count;

		for (int pos = 0; pos < count; pos++)
		{
			const t_PatternGroupe3<Traits>& pattern = list_groupe.patterns[pos];
			int score = 0;

			if (pattern.oposant_pos[1] == -1
			    || get_bb_flate<Traits>(opponent, pattern.oposant_pos[1]))
				score = 256;
			else if (pattern.oposant_pos[0] == -1
			         || get_bb_flate<Traits>(opponent, pattern.oposant_pos[0]))
				score = 128;
			if (pattern.oposant_pos[2] == -1
			    || get_bb_flate<Traits>(opponent, pattern.oposant_pos[2]))
				score += 256;
			else if (pattern.oposant_pos[3] == -1
			         || get_bb_flate<Traits>(opponent, pattern.oposant_pos[3]))
				score += 128;

			if (score > 256)
				continue;

			// SCORE_3_FULL : 3 pierres contiguës — un AND sur 1–2 words.
			if (matchPattern(pattern.fullMask, stones))
			{
				total_score += score + 8;
				double_three += 1;
				continue;
			}

			if (pattern.oposant_pos[3] == -1
			    || get_bb_flate<Traits>(opponent, pattern.oposant_pos[3]))
				score += 128;
			else if (pattern.oposant_pos[4] == -1
			         || get_bb_flate<Traits>(opponent, pattern.oposant_pos[4]))
				score += 128;
			if (score > 256)
				continue;

			if (pattern.stone_pos[3] == -1) // regarde si il y a un three Hole
				continue;

			// SCORE_3_HOLE : trois pierres avec un trou — même match masqué.
			if (!get_bb_flate<Traits>(opponent, pattern.hole_pos[0])
			    && matchPattern(pattern.holeMask0, stones))
			{
				total_score += score + 12;
				double_three += 1;
				continue;
			}
			if (!get_bb_flate<Traits>(opponent, pattern.hole_pos[1])
			    && matchPattern(pattern.holeMask1, stones))
			{
				total_score += score + 12;
				double_three += 1;
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

template<typename Traits>
int BitboardTool<Traits>::check_super_four(const typename Traits::Bitboard& stones,
                                           const typename Traits::Bitboard& opponent,
                                           int col, int row) const
{
	return check_super_four_impl(stones, opponent, col, row, kPatternPrefilter);
}

template<typename Traits>
int BitboardTool<Traits>::check_super_four_reference(const typename Traits::Bitboard& stones,
                                                     const typename Traits::Bitboard& opponent,
                                                     int col, int row) const
{
	return check_super_four_impl(stones, opponent, col, row, /*prefilter=*/false);
}

template<typename Traits>
int BitboardTool<Traits>::check_super_four_impl(const typename Traits::Bitboard& stones,
                                                const typename Traits::Bitboard& opponent,
                                                int col, int row, bool prefilter) const
{
	const int idx = idx_generic<Traits>(col, row);

	for (int dir = 0; dir < 4; dir++)
	{
		if (prefilter && popWindow(_window[idx][dir], stones) < 4)
			continue;

		const t_PatternList_super4<Traits>& list_super4 = _lts4[idx][dir];
		const int count = list_super4.count;

		for (int i = 0; i < count; i++)
		{
			const t_super4<Traits>& pattern = list_super4.patterns[i];

			if (matchPattern(pattern.mask, stones)
			    && !get_bb_flate<Traits>(opponent, pattern.hole_pos[0])
			    && !get_bb_flate<Traits>(opponent, pattern.hole_pos[1]))
				return 1;
		}
	}
	return 0;
}

template<typename Traits>
int BitboardTool<Traits>::check_cross(const typename Traits::Bitboard& stones,
                                      const typename Traits::Bitboard& opponent,
                                      int col, int row) const
{
	return check_cross_impl(stones, opponent, col, row, kPatternPrefilter);
}

template<typename Traits>
int BitboardTool<Traits>::check_cross_reference(const typename Traits::Bitboard& stones,
                                                const typename Traits::Bitboard& opponent,
                                                int col, int row) const
{
	return check_cross_impl(stones, opponent, col, row, /*prefilter=*/false);
}

template<typename Traits>
int BitboardTool<Traits>::check_cross_impl(const typename Traits::Bitboard& stones,
                                           const typename Traits::Bitboard& opponent,
                                           int col, int row, bool prefilter) const
{
	const int idx = idx_generic<Traits>(col, row);

	// Cross may be queried from centre or any arm. Unique own stones in the
	// union of the four ±4 windows: scoring crosses need ≥ 4 (DEMI_*).
	if (prefilter)
	{
		typename Traits::Bitboard neigh {};
		for (int dir = 0; dir < 4; ++dir)
		{
			const CompactMask<Traits>& w = _window[idx][dir];
			for (uint8_t i = 0; i < w.words; ++i)
				neigh[static_cast<size_t>(w.w + i)] |= w.m[i];
		}
		int n = 0;
		for (size_t i = 0; i < neigh.size(); ++i)
			n += __builtin_popcountll(neigh[i] & stones[i]);
		if (n < 4)
			return 0;
	}

	const t_PatternList_Cross& list_cross = _ltcross[idx];

	for (int p = 0; p < list_cross.count; p++)
	{
		const t_cross& cross = list_cross.cross[p];
		int opposant_score = 0;
		int score = 0;

		if (get_bb_flate<Traits>(opponent, cross.middle)
		    || get_bb_flate<Traits>(opponent, cross.up)
		    || get_bb_flate<Traits>(opponent, cross.down)
		    || get_bb_flate<Traits>(opponent, cross.left)
		    || get_bb_flate<Traits>(opponent, cross.right))
			continue;

		if (cross.opposant_up[1] == -1
		    || get_bb_flate<Traits>(opponent, cross.opposant_up[1]))
			opposant_score = 64;
		else if (cross.opposant_up[0] == -1
		         || get_bb_flate<Traits>(opponent, cross.opposant_up[0]))
			opposant_score = 32;
		if (cross.opposant_down[1] == -1
		    || get_bb_flate<Traits>(opponent, cross.opposant_down[1]))
			opposant_score += 64;
		else if (cross.opposant_down[0] == -1
		         || get_bb_flate<Traits>(opponent, cross.opposant_down[0]))
			opposant_score += 32;
		if (cross.opposant_left[1] == -1
		    || get_bb_flate<Traits>(opponent, cross.opposant_left[1]))
			opposant_score += 64;
		else if (cross.opposant_left[0] == -1
		         || get_bb_flate<Traits>(opponent, cross.opposant_left[0]))
			opposant_score += 32;
		if (cross.opposant_right[1] == -1
		    || get_bb_flate<Traits>(opponent, cross.opposant_right[1]))
			opposant_score += 64;
		else if (cross.opposant_right[0] == -1
		         || get_bb_flate<Traits>(opponent, cross.opposant_right[0]))
			opposant_score += 32;

		if (opposant_score > 64)
			continue;

		if (get_bb_flate<Traits>(stones, cross.middle))
			score += 3;
		if (get_bb_flate<Traits>(stones, cross.up))
			score += 4;
		if (get_bb_flate<Traits>(stones, cross.down))
			score += 4;
		if (get_bb_flate<Traits>(stones, cross.left))
			score += 4;
		if (get_bb_flate<Traits>(stones, cross.right))
			score += 4;

		if (score >= 15)
			return score + opposant_score;
	}
	return 0;
}

template<typename Traits>
bool BitboardTool<Traits>::isDoubleThreeScore(const int score) const
{
	if (score <= 0)
		return false;
	if (score < SCORE_3_FULL)
		return true;
	if (score >= SCORE_DOUBLE_FULL_FULL_EXTERN
	    && score <= SCORE_DOUBLE_HOLE_HOLE_EXTERN)
		return true;
	// if (score == SCORE_3_FULL || score == SCORE_3_HOLE || score >= SCORE_FULL_EXTERN)
	// 	return false;
	return false;
}

template<typename Traits>
bool BitboardTool<Traits>::is_double_three_at(const typename Traits::Bitboard& stones,
                                              const typename Traits::Bitboard& opponent,
                                              int col, int row) const
{
	const int score = check_open_three(stones, opponent, col, row);
	return isDoubleThreeScore(score);
}


template<typename Traits>
CaptureResult<Traits> BitboardTool<Traits>::resolveCaptures(t_BWBoard<Traits>& bb, int col,
                                                              int row, const Color color) const
{
    typename Traits::Bitboard capturedMask = {};

    detect_captures<Traits>(bb, col, row, color, capturedMask);

    const int newCaptures = popcount_bb_generic<Traits>(capturedMask);

    return { capturedMask, newCaptures };
}
