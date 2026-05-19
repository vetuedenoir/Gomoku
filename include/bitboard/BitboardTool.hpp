#ifndef BITBOARDTOOL_HPP
#define BITBOARDTOOL_HPP

#include "game/board/GameBoard.hpp"

class IBitboardTool
{
public:
	virtual ~IBitboardTool() = default;

	// virtual bool is_five_in_a_row(const GameBoard& board, const int col, const int row) = 0;
	// virtual bool is_open_four(const GameBoard& board, const int col, const int row) = 0;
	// virtual bool is_broken_four(const GameBoard& board, const int col, const int row) = 0;
	// virtual bool is_open_three(const GameBoard& board, const int col, const int row) = 0;
	// virtual bool is_broken_three(const GameBoard& board, const int col, const int row) = 0;
	// virtual bool is_cross(const GameBoard& board, const int col, const int row) = 0;
};

#endif // BITBOARDTOOL_HPP