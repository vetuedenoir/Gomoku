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

bool GameBoard::placeStone(int col, int row)
{
    if (col < 0 || col >= _size || row < 0 || row >= _size)
        return false;
    if (!isFree(col, row))
        return false;

    _board[row][col] = (_currentPlayer == 0) ? CellStatus::Black : CellStatus::White;
    _currentPlayer = 1 - _currentPlayer;  // flip between 0 and 1
    return true;
}

CellStatus GameBoard::getCell(int col, int row) const
{
    return _board[row][col];
}

int GameBoard::getCurrentPlayer() const
{
    return _currentPlayer;
}
