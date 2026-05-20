#ifndef MOVEVALIDATOR_HPP
# define MOVEVALIDATOR_HPP

#include "game/GameState.hpp"
#include "game/contracts/Move.hpp"
#include "game/rules/OpeningRules.hpp"
#include "game/rules/StandardRules.hpp"
#include "ai/MoveGenerator.hpp"
#include "bitboard/bitboard.hpp"
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
            {
                const bool ok = canPlaceOpeningStone(state, move);
                if (!ok)
                    Logger::warn("VALIDATOR",
                        "opening (" + std::to_string(move.col) + "," + std::to_string(move.row)
                        + ") rejected");
                else
                    Logger::debug("VALIDATOR",
                        "opening (" + std::to_string(move.col) + "," + std::to_string(move.row)
                        + ") ok");
                return ok;
            }
            case GamePhase::Standard:
            {
                const Color color = sideToMove(state);
                const bool  ok  = isLegalStandard(state, move.col, move.row, color);
                if (!ok)
                    Logger::warn("VALIDATOR",
                        "Standard (" + std::to_string(move.col) + "," + std::to_string(move.row)
                        + ") rejected");
                else
                    Logger::debug("VALIDATOR",
                        "Standard (" + std::to_string(move.col) + "," + std::to_string(move.row)
                        + ") ok");
                return ok;
            }
            case GamePhase::ColorChoice:
            default:
                return false;
        }
    }

    std::vector<Move> getLegalMoves(const GameState& state) const
    {
        switch (state.phase)
        {
            case GamePhase::Opening:
                return getLegalOpeningMoves(state);
            // case GamePhase::Standard:
            //     return getLegalStandardMoves(state);
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

    bool isLegalStandard(const GameState& state, int col, int row, Color color) const
    {
        const t_BWBoard<Traits> bb = GameBoard_to_bitboard<Traits>(*state.board);
        return _standardRules.isLegal(bb, col, row, color);
    }

    StandardRules<Traits> _standardRules;
};

#endif
