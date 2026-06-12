#ifndef OPENINGMOVEPOLICY_HPP
# define OPENINGMOVEPOLICY_HPP

#include "game/contracts/contracts.hpp"
#include "game/validation/ValidationContext.hpp"
#include "game/validation/rules/OpeningRules.hpp"

struct OpeningMovePolicy
{
    static bool isLegal(const ValidationContext& ctx, const Move& move)
    {
        return canPlaceOpeningStone(ctx.opening, ctx.board, move);
    }

    static std::vector<Move> legalMoves(const ValidationContext& ctx)
    {
        return getLegalOpeningMoves(ctx.opening, ctx.board);
    }
};

#endif
