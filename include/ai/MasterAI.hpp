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

using t_BWBoard_variant = std::variant<t_BWBoard19, t_BWBoard15>;

struct SearchStats
{
    int    nodesVisited   = 0;  // total minimax() calls
    int    nodesEvaluated = 0;  // leaf nodes reaching evaluatePosition()
    int    nodesPruned    = 0;  // alpha-beta cut-offs
    int    maxDepthSeen   = 0;  // deepest ply actually reached
    int    ttHits         = 0;  // TT entries reused with sufficient depth
    int    ttCutoffs      = 0;  // TT bound that produced an immediate cutoff
    int    ttStores       = 0;  // entries written to the TT
    int    bestScore      = 0;
    t_cell bestMove       = {-1, -1};
};

template <typename T> struct MasterAITestAccess;

struct sort_move_t
{
	t_cell			move;
	const TTEntry*		hit;
	position_hash_t	pos_hash;
};


template<typename Traits>
class MasterAI
{
	template <typename T> friend struct MasterAITestAccess;

	public:
		explicit MasterAI(int depth = 5, int activeZoneRadius = 1, Color aiColor = Color::Black);
		
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

		void protoTry(std::vector<t_cell>& Moves, const SearchPosition<Traits>& position);

		int minimax(
			SearchPosition<Traits>& position, t_cell cell,
			int depth, int alpha, int beta);
		
		int evaluatePosition(const SearchPosition<Traits>& position, t_cell cell);

		int evaluateBlackPosition(const SearchPosition<Traits>& position, t_cell cell);
		int evaluateWhitePosition(const SearchPosition<Traits>& position, t_cell cell);
};



static int getMoveScore(const sort_move_t& sm) {
    if (sm.hit == nullptr) return 0;
    if (sm.hit->flag == TTFlag::Exact)      return sm.hit->score + 1000000;
    if (sm.hit->flag == TTFlag::LowerBound) return sm.hit->score + 500000;
    return sm.hit->score;
}


using MasterAI19 = MasterAI<BoardTraits<19>>;
using MasterAI15 = MasterAI<BoardTraits<15>>;

#include "ai/MasterAI.inl"

#endif
