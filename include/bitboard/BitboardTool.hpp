#ifndef BITBOARDTOOL_HPP
#define BITBOARDTOOL_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/PatternTypes.hpp"

#include "game/contracts/contracts.hpp"

#include <cstddef>
#include <cstring>

template<typename Traits>
class BitboardTool
{
	t_PatternList5<Traits>        _lt5[Traits::CELL_COUNT][4];
	t_PatternList4<Traits>        _lt4[Traits::CELL_COUNT][4];
	t_PatternList_Groupe4<Traits> _ltg4[Traits::CELL_COUNT][4];
	t_PatternList_Groupe3<Traits> _lt3[Traits::CELL_COUNT][4];
	t_PatternList_super4<Traits>  _lts4[Traits::CELL_COUNT][4];
	t_PatternList_Cross           _ltcross[Traits::CELL_COUNT];

	// Inclusive word span of the non-zero limbs of `mask`. Empty mask → (0,0)
	// (matchPattern then trivially succeeds, same as before).
	static void computeMaskSpan(const typename Traits::Bitboard& mask,
	                            uint8_t& first, uint8_t& last)
	{
		first = 0;
		last  = 0;
		bool found = false;
		for (size_t i = 0; i < mask.size(); ++i)
		{
			if (mask[i] != 0)
			{
				if (!found)
				{
					first = static_cast<uint8_t>(i);
					found = true;
				}
				last = static_cast<uint8_t>(i);
			}
		}
	}

	// Match only the words the pattern actually occupies (typically 1–2 of 6
	// on 19×19). Semantically identical to scanning all words: zero limbs of
	// `pat` always succeed the subset test.
	static bool matchPattern(const typename Traits::Bitboard& pat,
	                         const typename Traits::Bitboard& board,
	                         uint8_t first, uint8_t last)
	{
		for (uint8_t i = first; i <= last; ++i)
		{
			if ((pat[i] & board[i]) != pat[i])
				return false;
		}
		return true;
	}

	// Fallback: skip zero limbs (no precomputed span).
	static bool matchPattern(const typename Traits::Bitboard& pat,
	                         const typename Traits::Bitboard& board)
	{
		for (size_t i = 0; i < pat.size(); ++i)
		{
			const uint64_t p = pat[i];
			if (p != 0 && (p & board[i]) != p)
				return false;
		}
		return true;
	}

	void buildAll();
	void build_lookup_table5();
	void build_lookup_table4();
	void build_lookup_table_groupe4();
	void build_lookup_table3();
	void build_lookup_table_super4();
	void build_lookup_table_cross();

	void add_mask_to_lookup5(const typename Traits::Bitboard& mask, int start_pos, int stride);
	void add_mask_to_lookup4(t_Pattern4<Traits>* pattern, int start_pos, int stride);
	void add_pattern_group4(t_PatternGroup4<Traits>* group, int start_x, int start_y, int dir);
	void add_pattern_group3_to_lookup(t_PatternGroupe3<Traits>* group, int dir);
	void finalize_group3_masks(t_PatternGroupe3<Traits>& pattern);
	void add_pattern_super4(t_super4<Traits>* pattern, int start_x, int start_y, int dir);
	void add_pattern_cross_to_lookup(t_cross* pattern);


public:
	BitboardTool() { buildAll(); }

	static BitboardTool<Traits>& instance()
	{
		static BitboardTool tool;
		return tool;
	}

	bool is_five_in_a_row(const typename Traits::Bitboard& stones, int col, int row) const;
	int check_open_four(const typename Traits::Bitboard& stones,
	                    const typename Traits::Bitboard& opponent, int col, int row) const;
	int check_broken_four(const typename Traits::Bitboard& stones,
	                      const typename Traits::Bitboard& opponent, int col, int row) const;
	int check_open_three(const typename Traits::Bitboard& stones,
	                     const typename Traits::Bitboard& opponent, int col, int row) const;
	int check_super_four(const typename Traits::Bitboard& stones,
	                     const typename Traits::Bitboard& opponent, int col, int row) const;
	int check_cross(const typename Traits::Bitboard& stones,
	                const typename Traits::Bitboard& opponent, int col, int row) const;
	bool is_double_three_at(const typename Traits::Bitboard& stones,
	                        const typename Traits::Bitboard& opponent, int col, int row) const;

	bool isDoubleThreeScore(const int score) const;

	CaptureResult<Traits> resolveCaptures(t_BWBoard<Traits>& bb, int col,
                                                              int row, const Color color) const;
};

using BitboardTool19 = BitboardTool<BoardTraits<19>>;
using BitboardTool15 = BitboardTool<BoardTraits<15>>;

#include "bitboard/BitboardToolBuild.inl"
#include "bitboard/BitboardToolChecks.inl"

#endif
