#pragma once

#include "bitboard/bitboard.hpp"

/* Move t_cell */
static bool contains_move(const std::vector<t_cell>& moves, int x, int y)
{
	for (const auto& m : moves)
		if (m.x == x && m.y == y)
			return true;
	return false;
}

static void place(GameBoard& b, int col, int row, CellStatus color)
{
    b.placeStoneOfColor(col, row, color);
}
