#ifndef MASTER_AI_HPP
# define MASTER_AI_HPP

#include "game/board/GameBoard.hpp"
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
		explicit MasterAI(int depth = 5, int activeZoneRadius = 1, Color aiColor = Color::Black);
		
		// Trouve le meilleur coup via minimax
		t_cell findBestMove(
			const SearchPosition<Traits>& position,
			Color color
		);
		
		void setSearchDepth(int depth) noexcept;
		int  getSearchDepth() const noexcept;

		int  getStonesCapturedByAI() const noexcept { return _stoneCapturedByAI; }
		void setStonesCapturedByAI(int count) noexcept { _stoneCapturedByAI = count; }

		int  getStonesCapturedByOPP() const noexcept { return _stoneCapturedByOPP;}
		void setStonesCapturedByOPP(int count) noexcept { _stoneCapturedByOPP = count; }

		Color getAIColor() const noexcept { return _aiColor; }
		void setAIColor(Color color) noexcept { _aiColor = color; }

		void setTimeLimit(int milliseconds) noexcept;
		void disableTimeLimit() noexcept;
		bool hasTimeLimit() const noexcept;

		const SearchStats& lastSearchStats() const noexcept { return _stats; }

	private:
		int                     _maxDepth;
		int					 	_stoneCapturedByAI = 0;
		int					 	_stoneCapturedByOPP = 0;
		Color                   _aiColor      = Color::Black;
		MoveGenerator<Traits>   _moveGenerator;
		
		using Clock     = std::chrono::steady_clock;
		using TimePoint = std::chrono::time_point<Clock>;

		SearchStats             _stats;
		std::optional<int>      _timeLimitMs = 1000;
		bool                    _timeExceeded = false;
		TimePoint               _searchStart  = {};

		void tickTimeLimit();

		int signedFromAi(Color side, int raw) const;

		int minimax(
			SearchPosition<Traits>& position, t_cell cell,
			int depth, int alpha, int beta, bool isMaximizing,
			std::vector<t_cell>& pv);
		
		int evaluatePosition(const SearchPosition<Traits>& position, t_cell cell);

		int evaluateBlackPosition(const SearchPosition<Traits>& position, t_cell cell);
		int evaluateWhitePosition(const SearchPosition<Traits>& position, t_cell cell);
};

using MasterAI19 = MasterAI<BoardTraits<19>>;
using MasterAI15 = MasterAI<BoardTraits<15>>;

#include "ai/MasterAI.inl"

#endif
