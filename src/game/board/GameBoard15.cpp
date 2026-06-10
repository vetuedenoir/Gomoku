#include "game/board/GameBoard15.hpp"
#include <algorithm>

GameBoard15::GameBoard15(int size, Seat firstPlayer)
    : _currentPlayer(firstPlayer), _size(size)
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

Seat GameBoard15::currentSeat() const
{
    return _currentPlayer;
}

void GameBoard15::setCurrentPlayer(Seat player)
{
    _currentPlayer = player;
}

void GameBoard15::switchPlayer()
{
    _currentPlayer = otherSeat(_currentPlayer);
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
