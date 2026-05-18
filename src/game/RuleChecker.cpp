#include "game/RuleChecker.hpp"
#include "bitboard/pattern.hpp"

bool RuleChecker::isLegal(const t_BWBoard19& board, int col, int row, Color color) const
{
    if (!in_board(col, row))
        return false;

    int pos = index_bb19(col, row);
    if (get_bb19(board.black, pos) || get_bb19(board.white, pos))
        return false;

    if (is_double_three_move(board, col, row, color))
    {
        // Double-three is legal only when the move simultaneously captures
        // at least one opponent stone (capture exception).
        t_BWBoard19 temp = board;
        set_bb19((color == Color::Black) ? temp.black : temp.white, col, row);
        bitboard19 captureMask = {};
        if (!detect_captures(temp, col, row, color, captureMask))
            return false;
    }

    return true;
}
