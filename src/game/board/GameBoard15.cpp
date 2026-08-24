#include "game/board/GameBoard15.hpp"
#include <algorithm>

GameBoard15::GameBoard15(int size, Color currentColor) : _currentColor(currentColor), _size(size)
{
	std::memset(_board, 0, sizeof(_board));
}

bool GameBoard15::isFree(int col, int row) const
{
	return _board[row][col] == CellStatus::Empty;
}

bool GameBoard15::isInside(int col, int row) const
{
	return (col >= 0 && row >= 0 && col < _size && row < _size);
}

CellStatus GameBoard15::getCell(int col, int row) const
{
	return _board[row][col];
}

int GameBoard15::getSize() const
{
	return _size;
}

Color GameBoard15::currentColor() const
{
	return _currentColor;
}

void GameBoard15::setCurrentColor(Color color)
{
	_currentColor = color;
}

void GameBoard15::clearCell(int col, int row)
{
	if (!isInside(col, row))
		return;
	_board[row][col] = CellStatus::Empty;
}

bool GameBoard15::placeStoneOfColor(int col, int row, CellStatus color)
{
	if (!isInside(col, row))
		return false;
	if (!isFree(col, row))
		return false;

	_board[row][col] = color;
	return true;
}
