#include "game/GameBoard.hpp"
#include <cstring>


GameBoard::GameBoard(int size, int firstPlayer) : _currentPlayer(firstPlayer), _size(size)
{
    std::memset(_board, 0, sizeof(_board));
}

bool GameBoard::isFree(int col, int row) const
{
    return _board[row][col] == CellStatus::Empty;
}

CellStatus GameBoard::getCell(int col, int row) const
{
    return _board[row][col];
}

int GameBoard::getSize() const
{
    return _size;
}

int GameBoard::getCurrentPlayer() const
{
    return _currentPlayer;
}

void GameBoard::setCurrentPlayer(int player)
{
    _currentPlayer = player;
}

void GameBoard::switchPlayer()
{
    _currentPlayer ^= 1;
}

bool GameBoard::placeStone(int col, int row)
{
    if (col < 0 || col >= _size || row < 0 || row >= _size)
        return false;
    if (!isFree(col, row))
        return false;

    _board[row][col] = (_currentPlayer == 0) ? CellStatus::Black : CellStatus::White;
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

int GameBoard::getSize() const
{
    return _size;
}

