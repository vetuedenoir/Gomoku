#ifndef PATTERN_TYPES_HPP
#define PATTERN_TYPES_HPP

#include "bitboard/bitboard.hpp"

#include <cstdint>

#define DIR_HORIZ 0
#define DIR_VERT  1
#define DIR_DIAG_G 2
#define DIR_DIAG_D 3

template<typename Traits>
struct t_PatternList5
{
	typename Traits::Bitboard masks[5];
	// Inclusive word range of non-zero limbs in masks[i] (for matchPattern).
	uint8_t firstWord[5];
	uint8_t lastWord[5];
	uint32_t count;
};

template<typename Traits>
struct t_super4
{
	typename Traits::Bitboard mask;
	uint8_t firstWord;
	uint8_t lastWord;
	int hole_pos[2];
};

template<typename Traits>
struct t_PatternList_super4
{
	t_super4<Traits> patterns[7];
	uint32_t count;
};

template<typename Traits>
struct t_Pattern4
{
	typename Traits::Bitboard mask;
	uint8_t firstWord;
	uint8_t lastWord;
	int opposant_left;
	int opposant_right;
};

template<typename Traits>
struct t_PatternList4
{
	t_Pattern4<Traits> patterns[4];
	int count;
};

template<typename Traits>
struct t_PatternGroup4
{
	typename Traits::Bitboard masks[3];
	uint8_t firstWord[3];
	uint8_t lastWord[3];
	int hole_pos[3];
};

template<typename Traits>
struct t_PatternList_Groupe4
{
	t_PatternGroup4<Traits> patterns[5];
	uint32_t count;
};

// Open-three pattern. stone_pos / hole_pos / oposant_pos stay as flat indices
// for openness checks; stone masks let check_open_three match the 3-stone
// shapes with a word-spanned AND instead of 3–4 get_bb_flate calls.
template<typename Traits>
struct t_PatternGroupe3
{
	int stone_pos[4];
	int hole_pos[2];
	int oposant_pos[5];

	// stones at [0],[1],[2]  → SCORE_3_FULL
	typename Traits::Bitboard fullMask{};
	uint8_t fullFirst = 0;
	uint8_t fullLast  = 0;

	// stones at [0],[2],[3] (hole at [1]) → SCORE_3_HOLE
	typename Traits::Bitboard holeMask0{};
	uint8_t hole0First = 0;
	uint8_t hole0Last  = 0;

	// stones at [0],[1],[3] (hole at [2]) → SCORE_3_HOLE
	typename Traits::Bitboard holeMask1{};
	uint8_t hole1First = 0;
	uint8_t hole1Last  = 0;
};

template<typename Traits>
struct t_PatternList_Groupe3
{
	t_PatternGroupe3<Traits> patterns[4];
	uint32_t count;
};

typedef struct
{
	int up;
	int down;
	int left;
	int right;
	int middle;
	int opposant_up[2];
	int opposant_down[2];
	int opposant_left[2];
	int opposant_right[2];
} t_cross;

typedef struct
{
	t_cross cross[5];
	int count;
} t_PatternList_Cross;

// Return values for open-three detection (check_open_three / check_three_align)

#define SCORE_3_FULL		8
#define SCORE_3_HOLE		12

#define SCORE_OPP_EXTERN	128
#define SCORE_OPP_INTERN	256

#define SCORE_FULL_EXTERN	(SCORE_3_FULL + SCORE_OPP_EXTERN) // 136
#define SCORE_HOLE_EXTERN	(SCORE_3_HOLE + SCORE_OPP_EXTERN) // 140

#define SCORE_FULL_INTERN	(SCORE_3_FULL + SCORE_OPP_INTERN) // 264
#define SCORE_HOLE_INTERN	(SCORE_3_HOLE + SCORE_OPP_INTERN) // 268

#define SCORE_DOUBLE_FULL_FULL			4
#define SCORE_DOUBLE_HOLE_FULL			5
#define SCORE_DOUBLE_HOLE_HOLE			6

#define SCORE_DOUBLE_FULL_FULL_EXTERN	36
#define SCORE_DOUBLE_HOLE_FULL_EXTERN	37
#define SCORE_DOUBLE_HOLE_HOLE_EXTERN	38

#define SCORE_DOUBLE_FULL_FULL_INTERN	68
#define SCORE_DOUBLE_HOLE_FULL_INTERN	69
#define SCORE_DOUBLE_HOLE_HOLE_INTERN	70

#define SCORE_DOUBLE_FULL_FULL_MIXED	100
#define SCORE_DOUBLE_HOLE_FULL_MIXED	101
#define SCORE_DOUBLE_HOLE_HOLE_MIXED	102

#define SCORE_DOUBLE_FULL_FULL_INTERN2	132
#define SCORE_DOUBLE_HOLE_FULL_INTERN2	133
#define SCORE_DOUBLE_HOLE_HOLE_INTERN2	134

// Return values for cross detection (check_cross)

#define CROSS_NONE				0
#define CROSS_DEMI_NO_MID		16
#define CROSS_DEMI_MID			15
#define CROSS_FULL				19

#define CROSS_OPP_EXTERN		32
#define CROSS_OPP_INTERN		64

#define CROSS_FULL_OPP_EXTERN	(CROSS_FULL + CROSS_OPP_EXTERN)
#define CROSS_FULL_OPP_INTERN	(CROSS_FULL + CROSS_OPP_INTERN)

#define CROSS_DEMI_NO_OPP_EXTERN	(CROSS_DEMI_NO_MID + CROSS_OPP_EXTERN)
#define CROSS_DEMI_NO_OPP_INTERN	(CROSS_DEMI_NO_MID + CROSS_OPP_INTERN)

#define CROSS_DEMI_MID_OPP_EXTERN	(CROSS_DEMI_MID + CROSS_OPP_EXTERN)
#define CROSS_DEMI_MID_OPP_INTERN	(CROSS_DEMI_MID + CROSS_OPP_INTERN)

#endif
