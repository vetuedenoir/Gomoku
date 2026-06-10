#include "game/board/GameBoard.hpp"
#include "game/board/GameBoard15.hpp"
#include "game/board/GameBoard19.hpp"

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
bool GameBoard::placeStoneOfColor(int col, int row, CellStatus color) { return _impl->placeStoneOfColor(col, row, color); }

CellStatus GameBoard::getCell(int col, int row) const { return _impl->getCell(col, row); }
Seat GameBoard::currentSeat() const { return _impl->currentSeat(); }
int GameBoard::getSize() const { return _impl->getSize(); }

void GameBoard::setCurrentPlayer(Seat player) { _impl->setCurrentPlayer(player); }
void GameBoard::switchPlayer() { _impl->switchPlayer(); }
void GameBoard::clearCell(int col, int row) { _impl->clearCell(col, row); }

#include "game/board/GameBoard.hpp"
