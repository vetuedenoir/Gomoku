#include "ai/MasterAI.hpp"
#include "game/turn/WinDetector.hpp"
#include "logger/Logger.hpp"
// les coups impliquant des captures doivent toujours etre mieux evalues que les coups sans capture.

// 1 MILLIONS = victoir
// superieur a 100 000 = coups imparables
// superieur a 10 000 = coups imparable dans les 2 prochains tours,
//						comme un double three ou une croix

// superieur a 1000 = coups avec 1 chance de parade, broken four

// superieur a 100 = coups avec 2 chances de parade,
//						comme une croix avec un seul coté ouvert, ou un three ouvert

// inferieur a 100 = coups avec 3 chances de parade ou plus, comme une croix avec les 2 cotés fermés, ou un three demi-ouvert

static int cross_score(int crossResult)
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

static int score_open_three(int threeResult)
{
	// LOG_DEBUG("AI", "[score_open_three] threeResult=" + std::to_string(threeResult));
    
	switch (threeResult)
    {
        case SCORE_3_FULL:  return 800;
        case SCORE_3_HOLE:  return 700;

        case SCORE_FULL_EXTERN: return 500;
        case SCORE_HOLE_EXTERN: return 450;

        case SCORE_FULL_INTERN: return 40;
		// peut etre meme en dessous de 100, car peut permettre une capture
        case SCORE_HOLE_INTERN: return 35;

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

// Score "brut" (non signé par rapport à l'IA) de la forme créée en posant
// `color` en `cell`. Les barèmes sont ceux de evaluatePosition, mais ici la
// pierre est posée de façon hypothétique dans une copie du plan de couleur
// (les masques des check_* incluent la case et exigent la pierre présente).


template<typename Traits>
void detect_and_stock_capture(const t_BWBoard<Traits>& board, int col, int row, const Color attackerColor, MoveList<t_cell, 16>& capturedStones)
{
	const typename Traits::Bitboard& attacker = bitboardForColor(board, attackerColor);
	const Color victimColor = (attackerColor == Color::Black) ? Color::White : Color::Black;
	const typename Traits::Bitboard& victime = bitboardForColor(board, victimColor);

	for (Direction dir : LINE_DIRS)
	{
		for (int sign : {-1, 1})
		{
			int stepX = sign * dx(dir);
			int stepY = sign * dy(dir);

			int x1 = col + 1 * stepX, y1 = row + 1 * stepY;
			int x2 = col + 2 * stepX, y2 = row + 2 * stepY;
			int x3 = col + 3 * stepX, y3 = row + 3 * stepY;

			if (!in_board_generic<Traits>(x1, y1) || !in_board_generic<Traits>(x2, y2) || !in_board_generic<Traits>(x3, y3))
				continue;

			if (get_bb_generic<Traits>(victime, x1, y1) &&
				get_bb_generic<Traits>(victime, x2, y2) &&
				get_bb_generic<Traits>(attacker, x3, y3))
			{
                capturedStones.push({x1, y1});
                capturedStones.push({x2, y2});
			}
		}
	}
}

int computeCaptureScore(EvaluatedMove& evaluatedMove)
{
	return evaluatedMove.capturedStones.size() * CAPTURE_SCORE * 2;
}

template <typename Traits>
EvaluatedMove MasterAI<Traits>::rawShapeScore(const t_BWBoard<Traits>& board, t_cell cell, Color color)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	// Pose hypothétiquement (copie locale)
	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove evaluatedMove {};
	
	evaluatedMove.isLegal = true;
	
	evaluatedMove.move = cell;
	
	detect_and_stock_capture(board, cell.x, cell.y, color, evaluatedMove.capturedStones);
	
	const int captureScore = computeCaptureScore(evaluatedMove);

	evaluatedMove.score = captureScore + 89;

	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		evaluatedMove.score = 1000000 + captureScore;
		return evaluatedMove;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	
	if (r == 2)
	{
		evaluatedMove.score = 500000 + captureScore;
		return evaluatedMove;
	}
	
	if (r == 1)
	{
		evaluatedMove.score = 5000 + captureScore;
		return evaluatedMove;
	}

	// Plus de quatre ouvert : la légalité suit la règle du double-trois, évaluée
	// via check_open_three exactement comme StandardRules::isLegal. Un quatre
	// brisé prime également sur le double-trois (coup légal) ; une capture lève
	// aussi l'interdiction du double-trois.
	const bool brokenFour = tool.check_broken_four(own, opp, cell.x, cell.y);
	const int  threeScore = brokenFour ? 0 : tool.check_open_three(own, opp, cell.x, cell.y);
	
	evaluatedMove.isLegal = brokenFour || !tool.isDoubleThreeScore(threeScore) || evaluatedMove.capturedStones.size() > 0;

	// Tri : super-four (60000) prioritaire sur le quatre brisé (6000), comme
	// dans l'heuristique de référence.
	if (tool.check_super_four(own, opp, cell.x, cell.y))
	{
		evaluatedMove.score = 60000 + captureScore;
		return evaluatedMove;
	}

	if (brokenFour)
	{
		evaluatedMove.score = 6000 + captureScore;
		return evaluatedMove;
	}

	r = tool.check_cross(own, opp, cell.x, cell.y);
	if (r)
	{
		evaluatedMove.score = cross_score(r) + captureScore;
		return evaluatedMove;
	}

	if (threeScore)
	{
		evaluatedMove.score = score_open_three(threeScore) + captureScore;
		return evaluatedMove;
	}

	return evaluatedMove;
}

// Clé de tri d'un candidat, du point de vue du camp au trait `side`.
// Variante complète : offense (ce que JE crée) + défense/2 (ce que l'adversaire
// créerait ici, donc valeur de blocage) + bonus de capture.
// Variante allégée (GOMOKU_LIGHT_MOVE_ORDER) : offense + captures uniquement,
// on économise un rawShapeScore par candidat.
// resolveCaptures n'exige pas la pierre posée (detect_captures lit
// victime-victime-attaquant depuis la case).
// template <typename Traits>
// int MasterAI<Traits>::staticMoveScore(const t_BWBoard<Traits>& board, t_cell cell, Color side)
// {
// 	const int off = rawShapeScore(board, cell, side);

// #ifdef GOMOKU_LIGHT_MOVE_ORDER
// 	return off + caps * CAPTURE_SCORE;
// #else
// 	const Color opp = (side == Color::Black) ? Color::White : Color::Black;
// 	const int def = rawShapeScore(board, cell, opp);
// 	return off + def / 2 + caps * CAPTURE_SCORE;
// #endif
// }

template <typename Traits>
int	MasterAI<Traits>::signedFromAi(Color side, int raw) const
{
	return (side == _aiColor) ? raw : -raw;
}

template <typename Traits>
int	MasterAI<Traits>::evaluatePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	int	result = 0;
	BitboardTool<Traits>& bitboardTool = BitboardTool<Traits>::instance();
	t_BWBoard<Traits> board = position.board();

	// makeMove already flipped sideToMove — the player who just placed at cell
	// is the OPPONENT of sideToMove().
	const Color side = (position.sideToMove() == Color::Black) ? Color::White : Color::Black;

	
	// ne verifie pas les captures gagnantes, seulement les alignements de 5
	if (isWinAfterMove<Traits>(board, side, cell.x, cell.y))
		return signedFromAi(side, 1000000);

	// n'enrigistre pas le compte de capture et ne modifie pas l'etat du board.
	// methode bientot obsolete, le nombre de capture sera enregistre dans le SearchPosition.
	const CaptureResult<Traits> caps = bitboardTool.resolveCaptures(board, cell.x, cell.y, side);
	if (caps.count)
		return signedFromAi(side, 200 * caps.count);

	if (side == Color::Black)
		result = bitboardTool.check_open_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_open_four(board.white, board.black, cell.x, cell.y);
	if (result == 2) // open four
		return signedFromAi(side, 500000);
	else if (result == 1) // half-open four
		return signedFromAi(side, 5000);

	if (side == Color::Black)
		result = bitboardTool.check_super_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_super_four(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, 60000);

	if (side == Color::Black)
		result = bitboardTool.check_broken_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_broken_four(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, 6000);

	if (side == Color::Black)
		result = bitboardTool.check_cross(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_cross(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, cross_score(result));

	if (side == Color::Black)
		result = bitboardTool.check_open_three(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_open_three(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, score_open_three(result));

	return 0;
}

template <typename Traits>
int MasterAI<Traits>::evaluateBlackPosition(
	const SearchPosition<Traits>& position,
	t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();
	const auto board = position.board();

	const int totalWhiteCaptures = position.getTotalwhiteCaptures();
	const int whiteCaptures =  (position.getWhiteCaptures()) ? 2 : 0;

	const int captureScore = whiteCaptures * CAPTURE_SCORE + totalWhiteCaptures;

	if (totalWhiteCaptures >= 10)
		return signedFromAi(Color::Black, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::Black, cell.x, cell.y))
		return signedFromAi(Color::Black, 1000000 + captureScore);

	int result = tool.check_open_four(board.black, board.white, cell.x, cell.y);
	if (result == 2)
		return signedFromAi(Color::Black, 500000 + captureScore);
	if (result == 1)
		return signedFromAi(Color::Black, 5000 + captureScore);

	if (tool.check_super_four(board.black, board.white, cell.x, cell.y))
		return signedFromAi(Color::Black, 60000 + captureScore);

	if (tool.check_broken_four(board.black, board.white, cell.x, cell.y))
		return signedFromAi(Color::Black, 6000 + captureScore);

	result = tool.check_cross(board.black, board.white, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::Black, cross_score(result) + captureScore);

	result = tool.check_open_three(board.black, board.white, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::Black, score_open_three(result) + captureScore);

	return captureScore;
}


template <typename Traits>
int MasterAI<Traits>::evaluateWhitePosition(
	const SearchPosition<Traits>& position,
	t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();
	const auto board = position.board();

	const int totalBlackCaptures = position.getTotalblackCaptures();
	const int blackCaptures = (position.getBlackCaptures()) ? 2 : 0;

	const int captureScore = blackCaptures * CAPTURE_SCORE + totalBlackCaptures;

	if (totalBlackCaptures >= 10)
		return signedFromAi(Color::White, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::White, cell.x, cell.y))
		return signedFromAi(Color::White, 1000000 + captureScore);

	int result = tool.check_open_four(board.black, board.white, cell.x, cell.y);
	if (result == 2)
		return signedFromAi(Color::White, 500000 + captureScore);
	if (result == 1)
		return signedFromAi(Color::White, 5000 + captureScore);

	if (tool.check_super_four(board.white, board.black, cell.x, cell.y))
		return signedFromAi(Color::White, 60000 + captureScore);

	if (tool.check_broken_four(board.white, board.black, cell.x, cell.y))
		return signedFromAi(Color::White, 6000 + captureScore);

	result = tool.check_cross(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::White, cross_score(result) + captureScore);

	result = tool.check_open_three(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::White, score_open_three(result) + captureScore);

	return captureScore;
}