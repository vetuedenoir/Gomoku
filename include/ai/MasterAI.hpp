#ifndef MASTER_AI_HPP
# define MASTER_AI_HPP

#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "game/validation/MoveValidator.hpp"
#include "bitboard/bitboard.hpp"
#include "SearchPosition.hpp"
#include "optimization/TranspositionTable.hpp"
#include "logger/Logger.hpp"
#include "config/config.hpp"
#include <optional>
#include <variant>
#include <limits>
#include <vector>
#include <chrono>
#include <algorithm>

using t_BWBoard_variant = std::variant<t_BWBoard19, t_BWBoard15>;

struct SearchStats
{
    int    nodesVisited   = 0;  // total minimax() calls
    int    nodesEvaluated = 0;  // leaf nodes reaching evaluatePosition()
    int    nodesPruned    = 0;  // alpha-beta cut-offs
    int    maxDepthSeen   = 0;  // deepest ply actually reached
    int    ttHits         = 0;  // [minimax] TT entries reused with sufficient depth
    int    ttCutoffs      = 0;  // [minimax] TT bound that produced an immediate cutoff
    int    ttStores       = 0;  // [minimax] entries written to the TT
    int    ttOrderingHits = 0;  // [minimax] TT bestMove hoisted to front for ordering
    int    ttRootHits         = 0;  // [root] probe found a matching entry
    int    ttRootOrderingHits = 0;  // [root] entry bestMove located & moved to front
    int    ttRootExactSeeds   = 0;  // [root] EXACT entry (depth>=maxDepth) seeded best move/score
    int    bestScore      = 0;
    t_cell bestMove       = {-1, -1};
};

template <typename T> struct MasterAITestAccess;

struct MoveSorted
{
	t_cell			    move;
	const TTEntry*		hit;
	MoveStateHash		stateHash;
};


template<typename Traits>
class MasterAI
{
	template <typename T> friend struct MasterAITestAccess;

	public:
		explicit MasterAI(int depth = 7, int activeZoneRadius = 1, Color aiColor = Color::Black);
		
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

		const SearchStats& lastSearchStats() const noexcept { return _stats; }

	private:
		int                     _maxDepth;
		int					 	_stoneCapturedByAI = 0;
		int					 	_stoneCapturedByOPP = 0;
		Color                   _aiColor      = Color::Black;
		MoveGenerator<Traits>   _moveGenerator;
		TranspositionTable      _tt;
		
		using Clock     = std::chrono::steady_clock;
		using TimePoint = std::chrono::time_point<Clock>;

		SearchStats             _stats;
	
		int signedFromAi(Color side, int raw) const;

		int minimax(SearchPosition<Traits>& position, t_cell cell, int depth, int alpha, int beta);
		
		int evaluatePosition(const SearchPosition<Traits>& position, t_cell cell);

		int evaluateBlackPosition(const SearchPosition<Traits>& position, t_cell cell);
		int evaluateWhitePosition(const SearchPosition<Traits>& position, t_cell cell);
		// int	staticMoveScore(const t_BWBoard<Traits>& board, t_cell cell, Color side);
		EvaluatedMove rawShapeScore(const t_BWBoard<Traits>& board, t_cell cell, Color color);
};

using MasterAI19 = MasterAI<BoardTraits<19>>;
using MasterAI15 = MasterAI<BoardTraits<15>>;

#include "ai/MasterAI.inl"
#include "ai/heuristique.inl"

#endif
