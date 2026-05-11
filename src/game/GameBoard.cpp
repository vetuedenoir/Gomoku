#include "game/GameBoard.hpp"
#include "game/GameBoard15.hpp"
#include "game/GameBoard19.hpp"

GameBoard::GameBoard(int size, Seat firstPlayer)
{
    if (size == 15)
        _impl = std::make_unique<GameBoard15>(size, firstPlayer);
    else
        _impl = std::make_unique<GameBoard19>(size, firstPlayer);
}

GameBoard::~GameBoard() = default;

bool GameBoard::isFree(int col, int row) const { return _impl->isFree(col, row); }
bool GameBoard::isInside(int col, int row) const { return _impl->isInside(col, row); }
bool GameBoard::placeStone(int col, int row) { return _impl->placeStone(col, row); }
bool GameBoard::placeStoneOfColor(int col, int row, CellStatus color) { return _impl->placeStoneOfColor(col, row, color); }

CellStatus GameBoard::getCell(int col, int row) const { return _impl->getCell(col, row); }
Seat GameBoard::currentSeat() const { return _impl->currentSeat(); }
int GameBoard::getSize() const { return _impl->getSize(); }

void GameBoard::setCurrentPlayer(Seat player) { _impl->setCurrentPlayer(player); }
void GameBoard::switchPlayer() { _impl->switchPlayer(); }
void GameBoard::clearCell(int col, int row) { _impl->clearCell(col, row); }
#include "game/GameBoard.hpp"

// GameBoard::GameBoard(int size, Seat firstPlayer)
//     : _currentPlayer(firstPlayer), _size(size), _board(size * size, CellStatus::Empty)
// {
// }

// static inline int idx(int col, int row, int size) { return row * size + col; }

// bool GameBoard::isFree(int col, int row) const
// {
//     return _board[idx(col, row, _size)] == CellStatus::Empty;
// }

// bool GameBoard::isInside(int col, int row) const
// {
//     return (col >= 0 && row >= 0 && col < _size && row < _size);
// }

// CellStatus GameBoard::getCell(int col, int row) const
// {
//     return _board[idx(col, row, _size)];
// }

// int GameBoard::getSize() const
// {
//     return _size;
// }

// Seat GameBoard::currentSeat() const
// {
//     return _currentPlayer;
// }

// void GameBoard::setCurrentPlayer(Seat player)
// {
//     _currentPlayer = player;
// }

// void GameBoard::switchPlayer()
// {
//     _currentPlayer = otherSeat(_currentPlayer);
// }

// bool GameBoard::placeStone(int col, int row)
// {
//     if (!isInside(col, row))
//         return false;
//     if (!isFree(col, row))
//         return false;

//     _board[idx(col, row, _size)] = (_currentPlayer == Seat::First) ? CellStatus::Black : CellStatus::White;
//     switchPlayer();
//     return true;
// }

// void GameBoard::clearCell(int col, int row)
// {
//     if (!isInside(col, row))
//         return;
//     _board[idx(col, row, _size)] = CellStatus::Empty;
// }

// bool GameBoard::placeStoneOfColor(int col, int row, CellStatus color)
// {
//     if (!isInside(col, row))
//         return false;
//     if (!isFree(col, row))
//         return false;

//     _board[idx(col, row, _size)] = color;
//     return true;
// }
