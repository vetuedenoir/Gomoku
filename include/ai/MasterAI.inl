#include "ai/MasterAI.hpp"
#include "game/turn/WinDetector.hpp"
#include "logger/Logger.hpp"

template <typename Traits>
MasterAI<Traits>::MasterAI(int depth, int activeZoneRadius)
	: _moveGenerator(activeZoneRadius), _maxDepth(depth)
{
}

template <typename Traits>
t_cell	MasterAI<Traits>::findBestMove(
	const SearchPosition<Traits>& position, Color color)
{
	bestescore = std::numeric_limits<int>::min();
	t_cell bestMove = {-1, -1};
	const std::vector<t_cell> moves = _moveGenerator.generateMoves(position.board(), position.sideToMove());
	if (moves.empty())
		return bestMove;
	
	// order moves by heuristic evaluation to improve alpha-beta pruning efficiency

	for (const t_cell& move : moves)
	{
		SearchPosition<Traits> newPosition = position;
		newPosition.makeMove(move.x, move.y, colorToCell(position.sideToMove()));
		int score = minimax(newPosition, move, _maxDepth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), false);
		if (score > bestescore)
		{
			bestescore = score;
			bestMove = move;
		}
	}
	return bestMove;
}

template <typename Traits>
void	MasterAI<Traits>::setSearchDepth(int depth) noexcept
{
	_maxDepth = depth;
}

template <typename Traits>
int	MasterAI<Traits>::getSearchDepth() const noexcept
{
	return _maxDepth;
}


template <typename Traits>
int	MasterAI<Traits>::minimax(SearchPosition<Traits>& position, t_cell cell,
			int depth, int alpha, int beta, bool isMaximizing)
{
	// ne verifie pas les captures gagnantes, seulement les alignements de 5
	if (isWinAfterMove<Traits>(position.board(), position.sideToMove(), 0, 0))
		return isMaximizing ? -1000000 : 1000000;
	
	if (depth == 0)
		return evaluatePosition(position, cell);
	
	const std::vector<t_cell> moves = _moveGenerator.generateMoves(position.board(), position.sideToMove());
	if (moves.empty())
		return evaluatePosition(position, cell);
	
	if (isMaximizing)
	{
		int maxEval = std::numeric_limits<int>::min();
		for (const t_cell& move : moves)
		{
			position.makeMove(move.x, move.y, colorToCell(position.sideToMove()));
			int eval = minimax(position, cell, depth - 1, alpha, beta, false);
			position.undoMove(move.x, move.y, colorToCell(position.sideToMove()));
			maxEval = std::max(maxEval, eval);
			alpha = std::max(alpha, eval);
			if (beta <= alpha)
				break;
		}
		return maxEval;
	}
	else
	{
		int minEval = std::numeric_limits<int>::max();
		for (const t_cell& move : moves)
		{
			position.makeMove(move.x, move.y, colorToCell(position.sideToMove()));
			int eval = minimax(position, cell, depth - 1, alpha, beta, true);
			position.undoMove(move.x, move.y, colorToCell(position.sideToMove()));
			minEval = std::min(minEval, eval);
			beta = std::min(beta, eval);
			if (beta <= alpha)
				break;
		}
		return minEval;
	}
}

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

template <typename Traits>
int	MasterAI<Traits>::evaluatePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	int	result = 0;
	BitboardTool<Traits>& bitboardTool = BitboardTool<Traits>::instance();
	t_BWBoard<Traits> board = position.board();
	Color side = position.sideToMove();

	// ne verifie pas les captures gagnantes, seulement les alignements de 5
	if (isWinAfterMove<Traits>(board, side, cell.x, cell.y))
		return 1000000;

	if (side == Color::Black)
		result = bitboardTool.check_open_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_open_four(board.white, board.black, cell.x, cell.y);
	if (result == 2) // open four
		return 500000;
	else if (result == 1) // half-open four
		return 5000;

	if (side == Color::Black)
		result = bitboardTool.check_super_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_super_four(board.white, board.black, cell.x, cell.y);
	if (result)
		return 60000;

	if (side == Color::Black)
		result = bitboardTool.check_broken_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_broken_four(board.white, board.black, cell.x, cell.y);	
	if (result)
		return 6000;

	if (side == Color::Black)
		result = bitboardTool.check_cross(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_cross(board.white, board.black, cell.x, cell.y);
	if (result)
		return (cross_score(result));

	
	if (side == Color::Black)
		result = bitboardTool.check_open_three(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_open_three(board.white, board.black, cell.x, cell.y);
	if (result)
		return score_open_three(result);

	return 0;
}