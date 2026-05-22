#ifndef MOVEVALIDATOR_HPP
# define MOVEVALIDATOR_HPP

// Uniform façade for move legality. Delegates by phase to policies with
// different internal representations. See include/game/validation/RULES.md.

#include "game/GameState.hpp"
#include "game/contracts/contracts.hpp"
#include "game/validation/policy/OpeningMovePolicy.hpp"
#include "game/validation/policy/StandardMovePolicy.hpp"
#include "logger/Logger.hpp"
#include <string>
#include <vector>

template<typename Traits>
class MoveValidator
{
public:
    bool isLegal(const GameState& state, const Move& move) const
    {
        switch (state.phase)
        {
            case GamePhase::Opening:
                return logResult("opening", move,
                    OpeningMovePolicy::isLegal(state, move));
            case GamePhase::Standard:
                return logResult("standard", move,
                    _standard.isLegal(state, move.col, move.row, sideToMove(state)));
            case GamePhase::ColorChoice:
            default:
                return false;
        }
    }

    std::vector<Move> legalMoves(const GameState& state) const
    {
        switch (state.phase)
        {
            case GamePhase::Opening:
                return OpeningMovePolicy::legalMoves(state);
            case GamePhase::Standard:
                return _standard.legalMoves(state, sideToMove(state));
            default:
                return {};
        }
    }

private:
    static Color sideToMove(const GameState& state)
    {
        const Seat seat = state.board->currentSeat();
        if (state.blackSeat.has_value())
            return (state.blackSeat.value() == seat) ? Color::Black : Color::White;
        return (seat == Seat::First) ? Color::Black : Color::White;
    }

    static bool logResult(const char* phase, const Move& move, bool ok)
    {
        const std::string coord = "(" + std::to_string(move.col) + ","
                                  + std::to_string(move.row) + ")";
        if (!ok)
            Logger::warn("VALIDATOR", std::string(phase) + " " + coord + " rejected");
        else
            Logger::debug("VALIDATOR", std::string(phase) + " " + coord + " ok");
        return ok;
    }

    StandardMovePolicy<Traits> _standard;
};

#endif
