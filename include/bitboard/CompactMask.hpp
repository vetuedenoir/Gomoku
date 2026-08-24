#ifndef COMPACT_MASK_HPP
#define COMPACT_MASK_HPP

#include "bitboard/bitboard.hpp"

#include <cassert>
#include <cstdint>

template<typename Traits> struct CompactMask
{
	static constexpr int MAX_WORDS = 4;

	uint64_t m[MAX_WORDS] = {};
	uint8_t  w            = 0;
	uint8_t  words        = 1;

	static CompactMask from(const typename Traits::Bitboard& bb)
	{
		CompactMask out;
		int         first = -1;
		int         last  = -1;
		for (int i = 0; i < Traits::WORD_COUNT; ++i)
		{
			if (bb[static_cast<size_t>(i)] != 0)
			{
				if (first < 0)
					first = i;
				last = i;
			}
		}
		if (first < 0)
			return out;

		const int span = last - first + 1;
		assert(span <= MAX_WORDS && "CompactMask: window spans more limbs than MAX_WORDS");

		out.w     = static_cast<uint8_t>(first);
		out.words = static_cast<uint8_t>(span);
		for (int i = 0; i < span; ++i)
			out.m[i] = bb[static_cast<size_t>(first + i)];
		return out;
	}

	typename Traits::Bitboard toBitboard() const
	{
		typename Traits::Bitboard out{};
		for (uint8_t i = 0; i < words; ++i)
			out[static_cast<size_t>(w + i)] = m[i];
		return out;
	}
};

template<typename Traits> inline int popWindow(const CompactMask<Traits>& p, const typename Traits::Bitboard& board)
{
	int n = 0;
	for (uint8_t i = 0; i < p.words; ++i)
		n += __builtin_popcountll(p.m[i] & board[p.w + i]);
	return n;
}

template<typename Traits> inline bool matchPattern(const CompactMask<Traits>& p, const typename Traits::Bitboard& board)
{
	for (uint8_t i = 0; i < p.words; ++i)
	{
		if ((p.m[i] & board[p.w + i]) != p.m[i])
			return false;
	}
	return true;
}

#endif
