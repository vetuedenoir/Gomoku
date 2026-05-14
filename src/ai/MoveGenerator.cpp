#include "ai/MoveGenerator.hpp"
#include <cstring>

MoveGenerator::MoveGenerator(int activeZoneRadius)
    : _activeZoneRadius(activeZoneRadius)
{}

void MoveGenerator::generateLegalMoves(const t_BWBoard19& board, Color color, bitboard19& legalMoves) const
{
    memset(legalMoves, 0, sizeof(bitboard19));

    ActiveZone zone(_activeZoneRadius);
    zone.initialize(board);

    const bitboard19& candidates = zone.getCandidateMask();

    for (int y = 0; y < 19; y++)
    {
        for (int x = 0; x < 19; x++)
        {
            if (get_bb19(candidates, index_bb19(x, y)) && isLegalMove(board, x, y, color))
                set_bb19(legalMoves, x, y);
        }
    }
}

bool MoveGenerator::isLegalMove(const t_BWBoard19& board, int col, int row, Color color) const
{
    if (!in_board(col, row))
        return false;

    int pos = index_bb19(col, row);
    if (get_bb19(board.black, pos) || get_bb19(board.white, pos))
        return false;

    if (is_double_three_move(board, col, row, color))
    {
        t_BWBoard19 temp = board;
        set_bb19((color == Color::Black) ? temp.black : temp.white, col, row);
        bitboard19 captureMask = {};
        if (!detect_captures(temp, col, row, color, captureMask))
            return false;
    }

    return true;
}
