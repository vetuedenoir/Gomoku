#ifndef PATTERN_HPP
#define PATTERN_HPP

#include "bitboard.hpp"
#include "string.h"

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
	bitboard19	mask;
	int			hole_pos[2];
}	t_super4;

typedef struct {
	t_super4	l_masks[7]; // 7 positions relatives de 4 pierres alignées, soit 7*128 = 896 bytes
	uint32_t	count;
}	t_MaskList_super4;

typedef struct {
	uint64_t masks[4][6]; //4 masks × 6 uint64_t = 192 bytes
	int  left_pos[4];	// 16 bytes
	int  right_pos[4];	// 16 bytes
	uint32_t count;	// 4 bytes
} t_MaskList4;

typedef struct {
	uint64_t	patterns[3][6];  // 3 patterns × 6 uint64_t = 144 bytes
	int			hole_pos[3];	// 12 bytes
} t_PatternGroup4; // 156 bytes total


typedef struct {
	t_PatternGroup4	masks[5];	// 780
	uint32_t		count;		// 4
	uint8_t			padding[16];
} t_MaskList_Groupe4; 	// 800 bytes total


// pattern en groupe de 3 pierres 

typedef struct {
	int		stone_pos[4];	// 16 bytes
	int		hole_pos[2];	// 8 bytes
	int		oposant_pos[4];	// 16 bytes
	int		count;	// 4 bytes
}	t_PatternGroupe3;

typedef struct {
	t_PatternGroupe3 patterns[4]; // 4 positions relatives de 3 pierres alignées, soit 4*44 = 176 bytes
	uint32_t count; // 4 bytes
} t_MaskList_Groupe3;

typedef struct {
	int	up;
	int	down;
	int	left;
	int	right;
	int	middle;
	int	opposant_up[2];
	int	opposant_down[2];
	int	opposant_left[2];
	int	opposant_right[2];
}	t_cross;

typedef struct {
	t_cross	cross[5]; // 5 positions relatives
	int		count;
}	t_PatternList_Cross;


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


// liste des patternes
	// 1. alignement de 5 pierres (gagne)
	// 2. alignement de 4 pierres partiellement ouvert (open four)
	// 3. alignement de 4 pierres completement ouvert (open four)
	// 4. alignement de 4 pierres troue (broken four)
	// [ ] [x] [x] [x] [x] [ ]  -> open four
	// [A] [x] [x] [x] [x] [ ]  -> partiel four
	// [ ] [x] [x] [x] [x] [A]  -> partiel four

	// [X] [ ] [x] [x] [x]  -> partiel four
	// [X] [X] [ ] [x] [x]  -> partiel four
	// [X] [X] [x] [ ] [x]  -> partiel four

	// [X] [ ] [X] [X] [x] [ ] [x]  -> open four indispendable 
	// [X] [ ] [x] [x] [x] [ ] [ ]  -> proto open four inutile
	// [ ] [ ] [X] [X] [x] [ ] [x]  -> proto open four inutile

	// 5. alignement de 3 pierres partiellement ouvert (open three)
	// 6. alignement de 3 pierres completement ouvert (open three)
	// 7. alignement de 3 pierres troue (broken three)
	// [ ] [x] [x] [x] [ ]  -> open three
	// [ ] [ ] [x] [x] [x] [ ] [ ] -> open three ???
	// [A] [ ] [x] [x] [x] [ ] [ ] -> open three ???
	// [ ] [ ] [x] [x] [x] [ ] [A] -> open three ???
	// [A] [ ] [x] [x] [x] [ ] [A] -> open three ???


	// [ ] [A] [x] [x] [x] [ ] [ ]-> partiel three
	// [ ] [ ] [x] [x] [x] [A] [ ]-> partiel three

	// [ ] [X] [ ] [x] [x] [ ]  -> partiel three ???
	// [ ] [x] [x] [ ] [x] [ ]  -> partiel three ???


	// 8 Les paterns en crois. legal si creer grace a capture.
	// [ ] [x] [ ]
	// [x] [x] [x]
	// [ ] [x] [ ]

	// [ ] [x] [ ]
	// [x] [ ] [x]
	// [ ] [x] [ ]

	// 9. Les paterns en T
	// [ ] [x] [ ]
	// [x] [x] [x]
	// [ ] [ ] [ ]
	// 10. Les paterns en L
	// [x] [ ]
	// [x] [ ]
	// [x] [ ]
	// [ ] [x]


	// double open three, legal seulement si les deux open three ne se croisent pas
	// ou si il est creer grace a une capture.
	// [A] [A] [ ]  [A] [A] [X]
	// [A] [X] [ ]  [A] [X] [ ]
	// [X] [A] [X]  [X] [A] [X]
	// [A] [A] [X]  [A] [A] [X]


	// T 3 2 : techniquement pas un double open tree
	// [X] [ ] [X]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// the T 4 3, potentiellement legal d'apres le sujet.
	//         [ ]
	// [ ] [X] [X] [X] [] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X]
	//         [X]
	//         [X]
	//         [A]

	// proto double three a demi ouvert
	// [X] [ ] [X]
	//     [X]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// double three a demi ouvert
	//         [ ]
	//         [ ]
	// [ ] [X] [X] [X] [ ] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X] 			si c'est a moi de jouer.
	//         [X]
	//         [A]

	// [ ] [x] [ ]
	// [x] [x] [x]
	// [ ] [ ] [ ]
	// 10. Les paterns en L
	// [x] [ ]
	// [x] [ ]
	// [x] [ ]
	// [ ] [x]


	// double open three, legal seulement si les deux open three ne se croisent pas
	// ou si il est creer grace a une capture.
	// [A] [A] [ ]  [A] [A] [X]
	// [A] [X] [ ]  [A] [X] [ ]
	// [X] [A] [X]  [X] [A] [X]
	// [A] [A] [X]  [A] [A] [X]


	// T 3 2 : techniquement pas un double open tree
	// [X] [ ] [X]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// the T 4 3, potentiellement legal d'apres le sujet.
	//         [ ]
	// [ ] [X] [X] [X] [] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X]
	//         [X]
	//         [X]
	//         [A]

	// proto double three a demi ouvert
	// [X] [ ] [X]
	//     [X]
	// [ ] [X] [ ]
	// [ ] [A] [ ]

	// double three a demi ouvert
	//         [ ]
	//         [ ]
	// [ ] [X] [X] [X] [ ] si encore un espace a gauche ou a droite, victoire assurer.
	//         [X] 			si c'est a moi de jouer.
	//         [X]
	//         [A]
