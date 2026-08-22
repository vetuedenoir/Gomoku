#ifndef COMPACT_MASK_HPP
# define COMPACT_MASK_HPP

#include "bitboard/bitboard.hpp"

#include <cassert>
#include <cstdint>

// Compact view of a Bitboard mask that spans a small run of consecutive limbs.
// Used for Lot-2 ±4 direction windows and Lot-3 pattern-table storage.
// On 19×19 a ±4 diagonal window can cross 4 limbs (MAX_WORDS).
template<typename Traits>
struct CompactMask
{
	static constexpr int MAX_WORDS = 4;

	uint64_t m[MAX_WORDS] = {};
	uint8_t  w     = 0; // index of the first occupied limb
	uint8_t  words = 1; // 1 .. MAX_WORDS

	// Build from a full bitboard. Empty mask → (w=0, words=1, m[0]=0).
	static CompactMask from(const typename Traits::Bitboard& bb)
	{
		CompactMask out;
		int first = -1;
		int last  = -1;
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

	// Inverse of from(): scatter the occupied limbs back into a full board.
	// Needed when a matched pattern must be handed to code that works on plain
	// Bitboards (e.g. the five-in-a-row mask fed to the capture-break check).
	typename Traits::Bitboard toBitboard() const
	{
		typename Traits::Bitboard out {};
		for (uint8_t i = 0; i < words; ++i)
			out[static_cast<size_t>(w + i)] = m[i];
		return out;
	}
};

// Stones of `board` that fall inside the compact window `p`.
template<typename Traits>
inline int popWindow(const CompactMask<Traits>& p, const typename Traits::Bitboard& board)
{
	int n = 0;
	for (uint8_t i = 0; i < p.words; ++i)
		n += __builtin_popcountll(p.m[i] & board[p.w + i]);
	return n;
}

// True iff every set bit of `p` is also set on `board`.
template<typename Traits>
inline bool matchPattern(const CompactMask<Traits>& p, const typename Traits::Bitboard& board)
{
	for (uint8_t i = 0; i < p.words; ++i)
	{
		if ((p.m[i] & board[p.w + i]) != p.m[i])
			return false;
	}
	return true;
}

#endif
