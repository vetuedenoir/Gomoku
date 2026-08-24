#pragma once

#include "bitboard/BitboardTool.hpp"
#include "bitboard/PatternTypes.hpp"
#include "bitboard/bitboard.hpp"

using TestTraits = BoardTraits<19>;

inline BitboardTool19& patternTool()
{
	return BitboardTool19::instance();
}

inline t_BWBoard19 empty_bb()
{
	return t_BWBoard19{};
}

inline void set_bb19(typename TestTraits::Bitboard& bb, int x, int y)
{
	set_bb_generic<TestTraits>(bb, x, y);
}

inline int index_bb19(int x, int y)
{
	return index_bb_generic<TestTraits>(x, y);
}

inline bool get_bb19(const typename TestTraits::Bitboard& bb, int idx)
{
	return get_bb_flate<TestTraits>(bb, idx);
}

inline int popcount_bb(const typename TestTraits::Bitboard& bb)
{
	return popcount_bb_generic<TestTraits>(bb);
}

inline bool isWin_ultra_19(const typename TestTraits::Bitboard& board, int x, int y)
{
	return patternTool().is_five_in_a_row(board, x, y);
}

inline int is_Open_4_19(const typename TestTraits::Bitboard& boardA, const typename TestTraits::Bitboard& boardB, int x,
                        int y)
{
	return patternTool().check_open_four(boardA, boardB, x, y);
}

inline int check_four_align_19(const typename TestTraits::Bitboard& boardA, const typename TestTraits::Bitboard& boardB,
                               int x, int y)
{
	return patternTool().check_broken_four(boardA, boardB, x, y);
}

inline int check_three_align_19(const typename TestTraits::Bitboard& boardA,
                                const typename TestTraits::Bitboard& boardB, int x, int y)
{
	return patternTool().check_open_three(boardA, boardB, x, y);
}

inline int check_super4_19(const typename TestTraits::Bitboard& boardA, const typename TestTraits::Bitboard& boardB,
                           int x, int y)
{
	return patternTool().check_super_four(boardA, boardB, x, y);
}

inline int check_cross_19(const typename TestTraits::Bitboard& boardA, const typename TestTraits::Bitboard& boardB,
                          int x, int y)
{
	return patternTool().check_cross(boardA, boardB, x, y);
}
