#pragma once

#include "bitboard/bitboard.hpp"
#include "ai/MoveGenerator.hpp"
#include <vector>

/* Move t_cell */
static bool contains_move(const std::vector<t_cell>& moves, int x, int y)
{
	for (const auto& m : moves)
		if (m.x == x && m.y == y)
			return true;
	return false;
}

template <typename Traits>
static std::vector<t_cell> legalMovesList(const MoveGenerator<Traits>& gen,
                                          const t_BWBoard<Traits>& board, Color color)
{
	std::vector<t_cell> out;
	typename Traits::Bitboard mask{};
	gen.getMaskOfLegalMoves(board, color, mask);
	bb_for_each_bit<Traits>(mask, [&](int x, int y) { out.push_back({x, y}); });
	return out;
}

static void place(GameBoard& b, int col, int row, CellStatus color)
{
    b.placeStoneOfColor(col, row, color);
}
