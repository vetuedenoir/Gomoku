#ifndef GAMEBOARD15_HPP
# define GAMEBOARD15_HPP

#include "game/GameBoard.hpp"
#include <cstring>

class GameBoard15 : public IGameBoard
{
    private:
        CellStatus _board[15][15];
        Seat       _currentPlayer;
        int        _size;

    public:
        GameBoard15(int size, Seat firstPlayer);

        bool       isFree(int col, int row) const override;
        bool       isInside(int col, int row) const override;
        bool       placeStone(int col, int row) override;
        bool       placeStoneOfColor(int col, int row, CellStatus color) override;

        CellStatus getCell(int col, int row) const override;
        Seat       currentSeat() const override;
        int        getSize() const override;

        void       setCurrentPlayer(Seat player) override;
        void       switchPlayer() override;
        void       clearCell(int col, int row) override;
};

#endif
