#ifndef STANDARDMOVEPOLICY_HPP
# define STANDARDMOVEPOLICY_HPP

#include "game/GameState.hpp"
#include "game/contracts/Move.hpp"
#include "game/contracts/Color.hpp"
#include "game/validation/rules/StandardRules.hpp"
#include "ai/MoveGenerator.hpp"
#include "bitboard/bitboard.hpp"
#include <vector>

// Standard-phase move checks. Ephemeral bitboard from GameBoard. See RULES.md.
template<typename Traits>
class StandardMovePolicy
{
public:
    bool isLegal(const GameState& state, int col, int row, Color color) const
    {
        return _rules.isLegal(boardFrom(state), col, row, color);
    }

    std::vector<Move> legalMoves(const GameState& state, Color color) const
    {
        std::vector<Move>           moves;
        const t_BWBoard<Traits>     bb   = boardFrom(state);
        typename Traits::Bitboard   mask = {};
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

private:
    static t_BWBoard<Traits> boardFrom(const GameState& state)
    {
        return GameBoard_to_bitboard<Traits>(*state.board);
    }

    StandardRules<Traits> _rules;
};

#endif
