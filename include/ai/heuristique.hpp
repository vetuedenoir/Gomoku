#ifndef HEURISTIQUE_HPP
#define HEURISTIQUE_HPP

#include "bitboard/PatternTypes.hpp"

// les coups impliquant des captures doivent toujours etre mieux evalues que les coups sans capture.

// 1 MILLIONS = victoir
// superieur a 100 000 = coups imparables
// superieur a 10 000 = coups imparable dans les 2 prochains tours,
//						comme un double three ou une croix

// superieur a 1000 = coups avec 1 chance de parade, broken four

// superieur a 100 = coups avec 2 chances de parade,
//						comme une croix avec un seul coté ouvert, ou un three ouvert

// inferieur a 100 = coups avec 3 chances de parade ou plus, comme une croix avec les 2 cotés fermés, ou un three demi-ouvert

inline int cross_score(int crossResult)
{
	switch (crossResult)
	{
		case CROSS_FULL:
		case CROSS_FULL_OPP_EXTERN:
			return 90000;

		case CROSS_DEMI_NO_MID:
			return 2000;

		case CROSS_DEMI_MID:
			return 1500;

		case CROSS_FULL_OPP_INTERN:
			return 9000;

		case CROSS_DEMI_NO_OPP_EXTERN:
			return 300;

		case CROSS_DEMI_MID_OPP_EXTERN:
			return 350;

		case CROSS_DEMI_NO_OPP_INTERN:
			return 150;

		case CROSS_DEMI_MID_OPP_INTERN:
			return 175;

		default:
			return 0;
	}
}

// Reflechir a enleve les verifications doublons CROSS_FULL et CROSS_FULL_OPP_EXTERN INTERN
// qui sont pareil que les double three SCORE_DOUBLE_FULL_FULL

inline int score_open_three(int threeResult)
{
	// LOG_DEBUG("AI", "[score_open_three] threeResult=" + std::to_string(threeResult));

	switch (threeResult)
	{
		case SCORE_3_FULL:
			return 800;
		case SCORE_3_HOLE:
			return 700;

		case SCORE_FULL_EXTERN:
			return 500;
		case SCORE_HOLE_EXTERN:
			return 450;

		case SCORE_FULL_INTERN:
			return 40;
		// peut etre meme en dessous de 100, car peut permettre une capture
		case SCORE_HOLE_INTERN:
			return 35;

		case SCORE_DOUBLE_FULL_FULL:
		case SCORE_DOUBLE_FULL_FULL_EXTERN:
			return 90000;

		case SCORE_DOUBLE_HOLE_FULL:
		case SCORE_DOUBLE_HOLE_FULL_EXTERN:
			return 85000;

		case SCORE_DOUBLE_HOLE_HOLE:
		case SCORE_DOUBLE_HOLE_HOLE_EXTERN:
			return 80000;

		// Necessite une defense en 3 coups pour etre pare, pas d'erreur possible
		case SCORE_DOUBLE_FULL_FULL_INTERN:
			return 9000;
		case SCORE_DOUBLE_HOLE_FULL_INTERN:
			return 8500;
		case SCORE_DOUBLE_HOLE_HOLE_INTERN:
			return 8000;

		// Necessite une defense en 2 coups pour etre pare, pas d'erreur possible
		case SCORE_DOUBLE_FULL_FULL_MIXED:
			return 7500;
		case SCORE_DOUBLE_HOLE_FULL_MIXED:
			return 7000;
		case SCORE_DOUBLE_HOLE_HOLE_MIXED:
			return 6500;

		case SCORE_DOUBLE_FULL_FULL_INTERN2:
			return 90;
		case SCORE_DOUBLE_HOLE_FULL_INTERN2:
			return 85;
		case SCORE_DOUBLE_HOLE_HOLE_INTERN2:
			return 80;

		default:
			return 0;
	}
}

#endif
