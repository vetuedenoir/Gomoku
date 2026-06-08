#include "ai/MasterAI.hpp"
#include "WinDetector.hpp"

template <typename Traits>
MasterAI<Traits>::MasterAI(int depth, int activeZoneRadius)
	: _moveGenerator(activeZoneRadius), _maxDepth(depth)
{
}

template <typename Traits>
std::pair<int, int>	MasterAI<Traits>::findBestMove(
	const SearchPosition<Traits>& position, Color color)
{
	int bestScore = std::numeric_limits<int>::min();
	std::pair<int, int> bestMove = {-1, -1};
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
		return isMaximizing ? -100000 : 100000;
	
	if (depth == 0)
		return evaluatePosition(position, cell);
	
	std::vector<t_cell> moves = _moveGenerator.generateMoves(position.board(), position.sideToMove());
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




static int	cross_score(int crossResult)
{
	if (crossResult == CROSS_FULL) // pareil a un double open three.
		return 90000;
	else if (crossResult == CROSS_DEMI_NO_MID) // 
		return 2000;
	else if (crossResult == CROSS_DEMI_MID)
		return 1500;
	else if (crossResult ==  CROSS_FULL_OPP_EXTERN)
		return 90000;
	else if (crossResult == CROSS_FULL_OPP_INTERN)
		return 9000;
	
	else if (crossResult == CROSS_DEMI_NO_OPP_EXTERN)
		return 300;
	else if (crossResult == CROSS_DEMI_NO_OPP_INTERN)
		return 150;
	
	else if (crossResult == CROSS_DEMI_MID_OPP_EXTERN)
		return 350;
	else if (crossResult = CROSS_DEMI_MID_OPP_INTERN)
		return 175;

	return 0;
}


static int	score_open_three(int threeResult)
{
	if (threeResult == SCORE_3_FULL)
		return 800;
	else if (threeResult == SCORE_3_HOLE)
		return 700;
	else if (threeResult == SCORE_FULL_EXTERN)
		return 500;
	else if (threeResult == SCORE_HOLE_EXTERN)
		return 450;
	else if (threeResult == SCORE_FULL_INTERN)
		return 400;
	// peut etre meme en dessous de 100, car peut permettre une capture
	else if (threeResult == SCORE_HOLE_INTERN)
		return 350;
	
	else if (threeResult == SCORE_DOUBLE_FULL_FULL)
		return 90000;
	else if (threeResult == SCORE_DOUBLE_HOLE_FULL)
		return 85000;
	else if (threeResult == SCORE_DOUBLE_HOLE_HOLE)
		return 80000;
	
	else if (threeResult == SCORE_DOUBLE_FULL_FULL_EXTERN)
		return 90000;
	else if (threeResult == SCORE_DOUBLE_HOLE_FULL_EXTERN)
		return 85000;
	else if (threeResult == SCORE_DOUBLE_HOLE_HOLE_EXTERN)
		return 80000;

	// Necessite une defense en 3 coups pour etre pare, pas d'erreur possible
	else if (threeResult == SCORE_DOUBLE_FULL_FULL_INTERN)
		return 9000;
	else if (threeResult == SCORE_DOUBLE_HOLE_FULL_INTERN)
		return 8500;
	else if (threeResult == SCORE_DOUBLE_HOLE_HOLE_INTERN)
		return 8000;

	else if (threeResult == SCORE_DOUBLE_FULL_FULL_MIXED)
		return 9000;
	else if (threeResult == SCORE_DOUBLE_HOLE_FULL_MIXED)
		return 8500;
	else if (threeResult == SCORE_DOUBLE_HOLE_HOLE_MIXED)
		return 8000;

}


template <typename Traits>
int	MasterAI<Traits>::evaluatePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	int score = 0;
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

}