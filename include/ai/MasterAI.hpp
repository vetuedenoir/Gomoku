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

using t_BWBoard_variant = std::variant<t_BWBoard19, t_BWBoard15>;

// class MasterIA
// {
//     public:
//         virtual ~MasterIA() = default;
// };


template<typename Traits>
class MasterAI
{
	public:
		explicit MasterAI(int depth = 6, int activeZoneRadius = 2);
		
		// Trouve le meilleur coup via minimax
		t_cell findBestMove(
			const SearchPosition<Traits>& position,
			Color color
		);
		
		void setSearchDepth(int depth) noexcept;
		int getSearchDepth() const noexcept;

	private:
		MoveGenerator<Traits>   _moveGenerator;
		int                     _maxDepth;
		
		// Minimax avec alpha-beta pruning
		int minimax(
			SearchPosition<Traits>& position, t_cell cell,
			int depth, int alpha, int beta, bool isMaximizing);
		
		int evaluatePosition(const SearchPosition<Traits>& position, t_cell cell);
};

using MasterAI19 = MasterAI<BoardTraits<19>>;
using MasterAI15 = MasterAI<BoardTraits<15>>;

#include "ai/MasterAI.inl"

#endif
