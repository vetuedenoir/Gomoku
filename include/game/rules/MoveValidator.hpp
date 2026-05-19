#ifndef MOVEVALIDATOR_HPP
# define MOVEVALIDATOR_HPP

#include "game/GameState.hpp"
#include "game/contracts/Move.hpp"
#include "game/rules/OpeningRules.hpp"
#include "game/rules/GomokuRules.hpp"
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
            case GamePhase::OpeningPlacement:
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
            case GamePhase::NormalPlay:
            {
                const Color color = sideToMove(state);
                const bool  ok  = isLegalNormal(state, move.col, move.row, color);
                if (!ok)
                    Logger::warn("VALIDATOR",
                        "normal (" + std::to_string(move.col) + "," + std::to_string(move.row)
                        + ") rejected");
                else
                    Logger::debug("VALIDATOR",
                        "normal (" + std::to_string(move.col) + "," + std::to_string(move.row)
                        + ") ok");
                return ok;
            }
            case GamePhase::ColorChoice:
            default:
                return false;
        }
    }

    std::vector<Move> legalMoves(const GameState& state) const
    {
        switch (state.phase)
        {
            case GamePhase::OpeningPlacement:
                return enumerateOpeningMoves(state);
            case GamePhase::NormalPlay:
                return legalMovesNormal(state);
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

    bool isLegalNormal(const GameState& state, int col, int row, Color color) const
    {
        const t_BWBoard<Traits> bb = GameBoard_to_bitboard<Traits>(*state.board);
        return _gomokuRules.isLegal(bb, col, row, color);
    }

    std::vector<Move> legalMovesNormal(const GameState& state) const
    {
        std::vector<Move>           moves;
        const Color                 color = sideToMove(state);
        const t_BWBoard<Traits>     bb    = GameBoard_to_bitboard<Traits>(*state.board);
        typename Traits::Bitboard   mask  = {};
        MoveGenerator<Traits>       gen(2);
        gen.generateLegalMoves(bb, color, mask);

        for (int y = 0; y < Traits::BOARD_SIZE; ++y)
        {
            for (int x = 0; x < Traits::BOARD_SIZE; ++x)
            {
                if (get_bb_generic<Traits>(mask, x, y))
                    moves.push_back({ x, y, CellStatus::Empty });
            }
        }
        return moves;
    }

    GomokuRules<Traits> _gomokuRules;
};

#endif
