#ifndef PATTERN_HPP
#define PATTERN_HPP

#include "bitboard.hpp"

#define	MAX_WINNING_MASK	1020
#define	MAX_FOUR_MASK	1120


#define DIR_HORIZ 0
#define DIR_VERT  1
#define DIR_DIAG_G 2  //
#define DIR_DIAG_D 3  //

#define IDX(x, y) ((y) * 19 + (x))


typedef struct {
	uint64_t masks[5][6];  // 5 masks × 6 uint64_t = 240 bytes
	uint32_t count;          // 4 bytes
	uint8_t padding[4]; // 12 bytes de padding pour aligner à 248 bytes
} t_MaskList5;  // 248 bytes total


typedef struct {
	uint64_t masks[4][6]; //4 masks × 6 uint64_t = 192 bytes
	int  left_pos[4];	// 16 bytes
	int  right_pos[4];	// 16 bytes
	uint32_t count;	// 4 bytes
	uint8_t padding[4];		// 4 bytes de padding pour aligner à 232 bytes
} t_MaskList4;

int	isWin_ultra(t_MaskList5 lookup_table5[361][4], const bitboard19& board,
	const int x, const int y);
int	is_Open_4(t_MaskList4 lookup_table4[361][4], const bitboard19 &boardA, const bitboard19 &boardB,
	const int x, const int y);

void	build_lookup_table5(t_MaskList5 lookup_table5[361][4], bitboard19 all_masks5[MAX_WINNING_MASK]);
void	build_lookup_table4(t_MaskList4 lookup_table4[361][4], bitboard19 all_masks4[MAX_FOUR_MASK]);

void	add_mask_to_lookup5(t_MaskList5 lookup_table5[361][4], bitboard19 all_masks5[MAX_WINNING_MASK],
	const int mask_index, const int start_pos, const int stride);
void	add_mask_to_lookup4(t_MaskList4 lookup_table4[361][4], bitboard19 all_masks4[MAX_FOUR_MASK],
	const int mask_index, const int start_pos, const int stride, const  int left_pos, const int right_pos);
		
void	test_bitboard(const GameBoard& board, int x, int y); // Fonction de test pour vérifier les patterns sur le bitboard

#endif // PATTERN_HPP