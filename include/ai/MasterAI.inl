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
		for (const auto& move : moves)
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
		for (const auto& move : moves)
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

template <typename Traits>
int	MasterAI<Traits>::evaluatePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	int score = 0;


	return score;



}