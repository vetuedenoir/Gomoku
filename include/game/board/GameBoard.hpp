#ifndef GAMEBOARD_HPP
# define GAMEBOARD_HPP

# include "game/board/Seat.hpp"
# include "game/contracts/contracts.hpp"
#include <memory>

// Abstract interface implemented by concrete fixed-size boards
class IGameBoard
{
    public:
        virtual ~IGameBoard() = default;

        virtual bool isFree(int col, int row) const = 0;
        virtual bool isInside(int col, int row) const = 0;
        virtual bool placeStoneOfColor(int col, int row, CellStatus color) = 0;

        virtual CellStatus getCell(int col, int row) const = 0;
        virtual Seat currentSeat() const = 0;
        virtual int getSize() const = 0;

        virtual void setCurrentPlayer(Seat player) = 0;
        virtual void switchPlayer() = 0;
        virtual void clearCell(int col, int row) = 0;
};

// Convenience concrete wrapper preserving the old `GameBoard` name.
// Internally delegates to a size-specific implementation (15 or 19).
class GameBoard : public IGameBoard
{
    public:
        GameBoard(int size, Seat firstPlayer);
        ~GameBoard();

        bool isFree(int col, int row) const override;
        bool isInside(int col, int row) const override;
        bool placeStoneOfColor(int col, int row, CellStatus color) override;

        CellStatus getCell(int col, int row) const override;
        Seat currentSeat() const override;
        int getSize() const override;

        void setCurrentPlayer(Seat player) override; // To delete, move to GameController
        void switchPlayer() override; // To delete
        void clearCell(int col, int row) override;

    private:
        std::unique_ptr<IGameBoard> _impl;
};

#endif
