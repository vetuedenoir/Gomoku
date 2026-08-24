#ifndef BITBOARDTOOL_HPP
#define BITBOARDTOOL_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/PatternTypes.hpp"
#include "bitboard/CompactMask.hpp"

#include "game/contracts/contracts.hpp"

#include <cstddef>
#include <cstring>

template<typename Traits> class BitboardTool
{
	t_PatternList5<Traits>        _lt5[Traits::CELL_COUNT][4];
	t_PatternList4<Traits>        _lt4[Traits::CELL_COUNT][4];
	t_PatternList_Groupe4<Traits> _ltg4[Traits::CELL_COUNT][4];
	t_PatternList_Groupe3<Traits> _lt3[Traits::CELL_COUNT][4];
	t_PatternList_super4<Traits>  _lts4[Traits::CELL_COUNT][4];
	t_PatternList_Cross           _ltcross[Traits::CELL_COUNT];

	CompactMask<Traits> _window[Traits::CELL_COUNT][4];

	void buildAll();
	void build_lookup_table5();
	void build_lookup_table4();
	void build_lookup_table_groupe4();
	void build_lookup_table3();
	void build_lookup_table_super4();
	void build_lookup_table_cross();
	void build_direction_windows();

	static constexpr bool kPatternPrefilter = true;

	bool is_five_in_a_row_impl(const typename Traits::Bitboard& stones, int col, int row, bool prefilter) const;
	int  find_five_masks_impl(const typename Traits::Bitboard& stones, int col, int row, typename Traits::Bitboard* out,
	                          int maxOut, bool prefilter) const;
	int  check_open_four_impl(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                          int col, int row, bool prefilter) const;
	int  check_broken_four_impl(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                            int col, int row, bool prefilter) const;
	int  check_open_three_impl(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                           int col, int row, bool prefilter) const;
	int  check_super_four_impl(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                           int col, int row, bool prefilter) const;
	int  check_cross_impl(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                      int row, bool prefilter) const;

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

	static constexpr int MAX_FIVE_MASKS = 5 * 4;

	bool is_five_in_a_row(const typename Traits::Bitboard& stones, int col, int row) const;

	int find_five_masks(const typename Traits::Bitboard& stones, int col, int row, typename Traits::Bitboard* out,
	                    int maxOut) const;
	int check_open_four(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                    int row) const;
	int check_broken_four(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                      int row) const;
	int check_open_three(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                     int row) const;
	int check_super_four(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                     int row) const;
	int check_cross(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                int row) const;

	bool is_five_in_a_row_reference(const typename Traits::Bitboard& stones, int col, int row) const;
	int  find_five_masks_reference(const typename Traits::Bitboard& stones, int col, int row,
	                               typename Traits::Bitboard* out, int maxOut) const;
	int  check_open_four_reference(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                               int col, int row) const;
	int  check_broken_four_reference(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                                 int col, int row) const;
	int  check_open_three_reference(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                                int col, int row) const;
	int  check_super_four_reference(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                                int col, int row) const;
	int  check_cross_reference(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                           int col, int row) const;

	int check_open_three_filtered(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent,
	                              int col, int row) const;

	bool is_double_three_at(const typename Traits::Bitboard& stones, const typename Traits::Bitboard& opponent, int col,
	                        int row) const;

	bool isDoubleThreeScore(const int score) const;

	CaptureResult<Traits> resolveCaptures(t_BWBoard<Traits>& bb, int col, int row, const Color color) const;

	const CompactMask<Traits>& directionWindow(int col, int row, int dir) const
	{
		return _window[idx_generic<Traits>(col, row)][dir];
	}
};

using BitboardTool19 = BitboardTool<BoardTraits<19> >;
using BitboardTool15 = BitboardTool<BoardTraits<15> >;

#include "bitboard/BitboardToolBuild.inl"
#include "bitboard/BitboardToolChecks.inl"

#endif
