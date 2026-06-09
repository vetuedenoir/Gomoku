#ifndef MASTER_AI_HPP
# define MASTER_AI_HPP

#include "game/board/GameBoard.hpp"
#include "game/GameState.hpp"
#include "game/contracts/contracts.hpp"
#include "game/validation/MoveValidator.hpp"
#include "bitboard/bitboard.hpp"
#include "SearchPosition.hpp"
#include "logger/Logger.hpp"
#include <optional>
#include <variant>
#include <limits>
#include <vector>
#include <chrono>

using t_BWBoard_variant = std::variant<t_BWBoard19, t_BWBoard15>;

struct SearchStats
{
    int    nodesVisited   = 0;  // total minimax() calls
    int    nodesEvaluated = 0;  // leaf nodes reaching evaluatePosition()
    int    nodesPruned    = 0;  // alpha-beta cut-offs
    int    maxDepthSeen   = 0;  // deepest ply actually reached
    int    bestScore      = 0;
    t_cell bestMove       = {-1, -1};

    std::vector<t_cell> principalVariation; // expected line of play
};

template<typename Traits>
class MasterAI
{
	public:
		explicit MasterAI(int depth = 4, int activeZoneRadius = 1);
		
		// Trouve le meilleur coup via minimax
		t_cell findBestMove(
			const SearchPosition<Traits>& position,
			Color color
		);
		
		void setSearchDepth(int depth) noexcept;
		int  getSearchDepth() const noexcept;

		void setTimeLimit(int milliseconds) noexcept;

		const SearchStats& lastSearchStats() const noexcept { return _stats; }

	private:
		using Clock     = std::chrono::steady_clock;
		using TimePoint = std::chrono::time_point<Clock>;

		MoveGenerator<Traits>   _moveGenerator;
		int                     _maxDepth;
		SearchStats             _stats;
		int                     _timeLimitMs  = 1000;
		bool                    _timeExceeded = false;
		TimePoint               _searchStart  = {};
		Color                   _aiColor      = Color::Black;

		int signedFromAi(Color side, int raw) const;

		int minimax(
			SearchPosition<Traits>& position, t_cell cell,
			int depth, int alpha, int beta, bool isMaximizing,
			std::vector<t_cell>& pv);
		
		int evaluatePosition(const SearchPosition<Traits>& position, t_cell cell);
};

using MasterAI19 = MasterAI<BoardTraits<19>>;
using MasterAI15 = MasterAI<BoardTraits<15>>;

#include "ai/MasterAI.inl"

#endif
