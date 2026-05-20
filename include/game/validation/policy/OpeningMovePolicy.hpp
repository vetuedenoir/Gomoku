#ifndef OPENINGMOVEPOLICY_HPP
# define OPENINGMOVEPOLICY_HPP

#include "game/GameState.hpp"
#include "game/contracts/Move.hpp"
#include "game/validation/rules/OpeningRules.hpp"
#include <vector>

// Opening-phase move checks. Uses GameBoard + OpeningRules (not bitboard).
// See RULES.md.
struct OpeningMovePolicy
{
    static bool isLegal(const GameState& state, const Move& move)
    {
        return canPlaceOpeningStone(state, move);
    }

    static std::vector<Move> legalMoves(const GameState& state)
    {
        return getLegalOpeningMoves(state);
    }
};

#endif
