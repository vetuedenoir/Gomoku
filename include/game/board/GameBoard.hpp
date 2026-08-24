#ifndef GAMEBOARD_HPP
#define GAMEBOARD_HPP

#include "game/contracts/contracts.hpp"
#include <memory>

// Abstract interface implemented by concrete fixed-size boards
class IGameBoard
{
public:
	virtual ~IGameBoard() = default;

	virtual int getSize() const = 0;

	virtual bool isFree(int col, int row) const                        = 0;
	virtual bool isInside(int col, int row) const                      = 0;
	virtual bool placeStoneOfColor(int col, int row, CellStatus color) = 0;

	virtual Color currentColor() const         = 0;
	virtual void  setCurrentColor(Color color) = 0;

	virtual CellStatus getCell(int col, int row) const = 0;
	virtual void       clearCell(int col, int row)     = 0;
};

class GameBoard : public IGameBoard
{
public:
	GameBoard(int size, Color currentColor = Color::Black);
	~GameBoard();

	GameBoard(GameBoard&&) noexcept;
	GameBoard& operator=(GameBoard&&) noexcept;

	int        getSize() const override;
	bool       isFree(int col, int row) const override;
	bool       isInside(int col, int row) const override;
	bool       placeStoneOfColor(int col, int row, CellStatus color) override;
	Color      currentColor() const override;
	void       setCurrentColor(Color color) override;
	CellStatus getCell(int col, int row) const override;
	void       clearCell(int col, int row) override;

private:
	std::unique_ptr<IGameBoard> _impl;
};

#endif
