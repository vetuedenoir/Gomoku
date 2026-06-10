#ifndef GAMEBOARD19_HPP
# define GAMEBOARD19_HPP

#include "game/board/GameBoard.hpp"
#include <cstring>

class GameBoard19 : public IGameBoard
{
    private:
        CellStatus _board[19][19];
        Seat       _currentPlayer;
        int        _size;

    public:
        GameBoard19(int size, Seat firstPlayer);

        bool       isFree(int col, int row) const override;
        bool       isInside(int col, int row) const override;
        bool       placeStoneOfColor(int col, int row, CellStatus color) override;

        CellStatus getCell(int col, int row) const override;
        Seat       currentSeat() const override;
        int        getSize() const override;

        void       setCurrentPlayer(Seat player) override;
        void       switchPlayer() override;
        void       clearCell(int col, int row) override;
};

#endif
