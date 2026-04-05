#include "game/GameBoard.hpp"
#include <cstring>


GameBoard::GameBoard() : _currentPlayer(0)
{
    std::memset(_board, 0, sizeof(_board));
}

bool GameBoard::isFree(int col, int row) const
{
    return _board[row][col][0] == 0 && _board[row][col][1] == 0;
}

bool GameBoard::placeStone(int col, int row)
{
    if (col < 0 || col >= 19 || row < 0 || row >= 19)
        return false;
    if (!isFree(col, row))
        return false;

    _board[row][col][_currentPlayer] = 1;
    _currentPlayer = 1 - _currentPlayer;  // flip between 0 and 1
    return true;
}

int GameBoard::getCell(int col, int row) const
{
    if (_board[row][col][0]) return 1;
    if (_board[row][col][1]) return 2;
    return 0;
}

int GameBoard::getCurrentPlayer() const
{
    return _currentPlayer;
}
