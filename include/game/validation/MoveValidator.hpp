#ifndef MOVEVALIDATOR_HPP
#define MOVEVALIDATOR_HPP

#include "game/contracts/contracts.hpp"
#include "game/board/GameBoard.hpp"
#include "game/validation/policy/StandardMovePolicy.hpp"
#include "logger/Logger.hpp"
#include <string>

template<typename Traits> class MoveValidator
{
public:
	bool isLegal(const GameBoard& board, Color sideToMove, const Move& move) const
	{
		const char* reason = nullptr;
		const bool  ok     = _standardPolicy.isLegal(board, move.col, move.row, sideToMove, &reason);
		return logResult(move, ok, reason);
	}

private:
	static bool logResult(const Move& move, bool ok, const char* reason)
	{
		const std::string coord = "(" + std::to_string(move.col) + "," + std::to_string(move.row) + ")";
		if (!ok)
		{
			std::string msg = "standard " + coord + " rejected";
			if (reason)
				msg += " — " + std::string(reason);
			Logger::warn("VALIDATOR", msg);
		}
		else
			LOG_DEBUG("VALIDATOR", "standard " + coord + " ok");
		return ok;
	}

	StandardMovePolicy<Traits> _standardPolicy;
};

#endif
