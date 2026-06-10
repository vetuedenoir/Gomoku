#include "game/board/GameBoard19.hpp"
#include <algorithm>

GameBoard19::GameBoard19(int size, Seat firstPlayer)
    : _currentPlayer(firstPlayer), _size(size)
{
    std::memset(_board, 0, sizeof(_board));
}

bool GameBoard19::isFree(int col, int row) const
{
    return _board[row][col] == CellStatus::Empty;
}

bool GameBoard19::isInside(int col, int row) const
{
    return (col >= 0 && row >= 0 && col < _size && row < _size);
}

CellStatus GameBoard19::getCell(int col, int row) const
{
    return _board[row][col];
}

int GameBoard19::getSize() const
{
    return _size;
}

Seat GameBoard19::currentSeat() const
{
    return _currentPlayer;
}

void GameBoard19::setCurrentPlayer(Seat player)
{
    _currentPlayer = player;
}

void GameBoard19::switchPlayer()
{
    _currentPlayer = otherSeat(_currentPlayer);
}

void GameBoard19::clearCell(int col, int row)
{
    if (!isInside(col, row))
        return;
    _board[row][col] = CellStatus::Empty;
}

bool GameBoard19::placeStoneOfColor(int col, int row, CellStatus color)
{
    if (!isInside(col, row))
        return false;
    if (!isFree(col, row))
        return false;

    _board[row][col] = color;
    return true;
}
