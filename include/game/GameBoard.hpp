#ifndef GAMEBOARD_HPP
# define GAMEBOARD_HPP

#include "game/Seat.hpp"

enum class CellStatus
{
    Empty = 0,
    Black = 1,
    White = 2
};

class GameBoard
{
    private:
        CellStatus _board[19][19];
        Seat       _currentPlayer;
        int        _size;

    public:
        GameBoard(int size, Seat firstPlayer);

        bool       isFree(int col, int row) const;
        bool       placeStone(int col, int row);
        bool       placeStoneOfColor(int col, int row, CellStatus color);

        CellStatus getCell(int col, int row) const;
        Seat       currentSeat() const;
        int        getSize() const;

        void       setCurrentPlayer(Seat player);
        void       switchPlayer();
        void       clearCell(int col, int row);  // used by SearchPosition::undoMove
};

#endif
