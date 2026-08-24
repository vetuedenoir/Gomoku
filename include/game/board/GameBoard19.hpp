#ifndef GAMEBOARD19_HPP
#define GAMEBOARD19_HPP

#include "game/board/GameBoard.hpp"
#include <cstring>

class GameBoard19 : public IGameBoard
{
private:
	CellStatus _board[19][19];
	Color      _currentColor;
	int        _size;

public:
	GameBoard19(int size, Color currentColor);

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
