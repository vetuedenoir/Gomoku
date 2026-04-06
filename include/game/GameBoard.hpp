#ifndef GAMEBOARD_HPP
# define GAMEBOARD_HPP

// Array layout: _board[row][col][player]
//   _board[row][col][0] = 1  →  player 0 (black) has a stone here
//   _board[row][col][1] = 1  →  player 1 (white) has a stone here
//   both == 0               →  cell is free

class GameBoard
{
private:
    char _board[19][19][2];
    int  _currentPlayer;
    int  _size;

public:
    GameBoard(int size, int firstPlayer);

    bool isFree(int col, int row) const;
    bool placeStone(int col, int row);
    int  getCell(int col, int row) const;
    int  getCurrentPlayer() const;
};

#endif
