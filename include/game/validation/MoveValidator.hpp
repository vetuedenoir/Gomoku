#ifndef MOVEVALIDATOR_HPP
# define MOVEVALIDATOR_HPP

#include "game/validation/ValidationContext.hpp"
#include "game/contracts/contracts.hpp"
#include "game/validation/policy/OpeningMovePolicy.hpp"
#include "game/validation/policy/StandardMovePolicy.hpp"
#include "logger/Logger.hpp"
#include "config/config.hpp"
#include <string>
#include <vector>

template<typename Traits>
class MoveValidator
{
public:
    bool isLegal(const ValidationContext& ctx, GamePhase phase,
                 Color sideToMove, const Move& move) const
    {
        switch (phase)
        {
            case GamePhase::Opening:
                return logResult("opening", move,
                    OpeningMovePolicy::isLegal(ctx, move));
            case GamePhase::Standard:
                return logResult("standard", move,
                    _standard.isLegal(ctx.board, move.col, move.row, sideToMove));
            case GamePhase::ColorChoice:
            default:
                return false;
        }
    }

    std::vector<Move> legalMoves(const ValidationContext& ctx,
                                 GamePhase phase,
                                 Color sideToMove) const
    {
        switch (phase)
        {
            case GamePhase::Opening:
                return OpeningMovePolicy::legalMoves(ctx);
            case GamePhase::Standard:
                return _standard.legalMoves(ctx.board, sideToMove);
            default:
                return {};
        }
    }

private:
    static bool logResult(const char* phase, const Move& move, bool ok)
    {
        const std::string coord = "(" + std::to_string(move.col) + ","
                                  + std::to_string(move.row) + ")";
        if (!ok)
            Logger::warn("VALIDATOR", std::string(phase) + " " + coord + " rejected");
        else
            LOG_DEBUG("VALIDATOR", std::string(phase) + " " + coord + " ok");
        return ok;
    }

    StandardMovePolicy<Traits> _standard;
};

#endif
