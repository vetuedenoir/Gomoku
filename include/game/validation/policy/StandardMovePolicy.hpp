#ifndef STANDARDMOVEPOLICY_HPP
# define STANDARDMOVEPOLICY_HPP

#include "game/contracts/contracts.hpp"
#include "game/validation/rules/StandardRules.hpp"
#include "game/board/GameBoard.hpp"
#include "ai/MoveGenerator.hpp"
#include "bitboard/bitboard.hpp"
#include <vector>

template<typename Traits>
class StandardMovePolicy
{
public:
    bool isLegal(const GameBoard& board, int col, int row, Color color) const
    {
        return _rules.isLegal(boardFrom(board), col, row, color);
    }

    std::vector<Move> legalMoves(const GameBoard& board, Color color) const
    {
        std::vector<Move>           moves;
        const t_BWBoard<Traits>     bb   = boardFrom(board);
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
    static t_BWBoard<Traits> boardFrom(const GameBoard& board)
    {
        return GameBoard_to_bitboard<Traits>(board);
    }

    StandardRules<Traits> _rules;
};

#endif
