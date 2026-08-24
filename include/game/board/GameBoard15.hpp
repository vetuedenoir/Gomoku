#ifndef GAMEBOARD15_HPP
#define GAMEBOARD15_HPP

#include "game/board/GameBoard.hpp"
#include <cstring>

class GameBoard15 : public IGameBoard
{
private:
	CellStatus _board[15][15];
	Color      _currentColor;
	int        _size;

public:
	GameBoard15(int size, Color currentColor);

	bool isFree(int col, int row) const override;
	bool isInside(int col, int row) const override;
	bool placeStoneOfColor(int col, int row, CellStatus color) override;

	CellStatus getCell(int col, int row) const override;
	Color      currentColor() const override;
	int        getSize() const override;

	void setCurrentColor(Color color) override;
	void clearCell(int col, int row) override;
};

#endif
