#include "game/GameBoard.hpp"
#include <cstring>

GameBoard::GameBoard(int size, Seat firstPlayer) : _currentPlayer(firstPlayer), _size(size)
{
    std::memset(_board, 0, sizeof(_board));
}

bool GameBoard::isFree(int col, int row) const
{
    return _board[row][col] == CellStatus::Empty;
}

bool GameBoard::isInside(int col, int row) const
{
    if ((col >= 0 && row >= 0) && (col <= _size && row <= _size)) {
        return true;
    }
    return false;
}

CellStatus GameBoard::getCell(int col, int row) const
{
    return _board[row][col];
}

int GameBoard::getSize() const
{
    return _size;
}

Seat GameBoard::currentSeat() const
{
    return _currentPlayer;
}

void GameBoard::setCurrentPlayer(Seat player)
{
    _currentPlayer = player;
}

void GameBoard::switchPlayer()
{
    _currentPlayer = otherSeat(_currentPlayer);
}

bool GameBoard::placeStone(int col, int row)
{
    if (col < 0 || col >= _size || row < 0 || row >= _size)
        return false;
    if (!isFree(col, row))
        return false;

    _board[row][col] = (_currentPlayer == Seat::First) ? CellStatus::Black : CellStatus::White;
    switchPlayer();
    return true;
}

bool GameBoard::placeStoneOfColor(int col, int row, CellStatus color)
{
    if (col < 0 || col >= _size || row < 0 || row >= _size)
        return false;
    if (!isFree(col, row))
        return false;

    _board[row][col] = color;
    return true;
}
