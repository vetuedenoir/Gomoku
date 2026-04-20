#ifndef GAMEBOARD_HPP
# define GAMEBOARD_HPP

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
        int        _currentPlayer;
        int        _size;

    public:
        GameBoard(int size, int firstPlayer);

        bool       isFree(int col, int row) const;
        bool       placeStone(int col, int row);
        bool       placeStoneOfColor(int col, int row, CellStatus color);

        CellStatus getCell(int col, int row) const;
        int        getCurrentPlayer() const;
        int        getSize() const;

        void       setCurrentPlayer(int player);
        void       switchPlayer();
};

#endif
