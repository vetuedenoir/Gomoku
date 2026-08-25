#include "game/board/GameBoard.hpp"
#include "game/board/GameBoard15.hpp"
#include "game/board/GameBoard19.hpp"

GameBoard::GameBoard(int size, Color currentColor)
{
	if (size == 15)
		_impl = std::make_unique<GameBoard15>(size, currentColor);
	else
		_impl = std::make_unique<GameBoard19>(size, currentColor);
}

GameBoard::~GameBoard() = default;

GameBoard::GameBoard(GameBoard&&) noexcept            = default;
GameBoard& GameBoard::operator=(GameBoard&&) noexcept = default;

int GameBoard::getSize() const
{
	return _impl->getSize();
}
bool GameBoard::isFree(int col, int row) const
{
	return _impl->isFree(col, row);
}
bool GameBoard::isInside(int col, int row) const
{
	return _impl->isInside(col, row);
}
bool GameBoard::placeStoneOfColor(int col, int row, CellStatus color)
{
	return _impl->placeStoneOfColor(col, row, color);
}

Color GameBoard::currentColor() const
{
	return _impl->currentColor();
}
void GameBoard::setCurrentColor(Color color)
{
	_impl->setCurrentColor(color);
}
CellStatus GameBoard::getCell(int col, int row) const
{
	return _impl->getCell(col, row);
}
void GameBoard::clearCell(int col, int row)
{
	_impl->clearCell(col, row);
}

#include "game/board/GameBoard.hpp"