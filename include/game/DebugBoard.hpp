// DebugBoard.hpp
#ifndef DEBUGBOARD_HPP
#define DEBUGBOARD_HPP

#include "game/GameBoard.hpp"
#include "ai/ActiveZone.hpp"
#include <vector>

enum class Cell { Empty = 0, Black = 1, White = 2, OOB = 3, Scan = 4 };

inline Cell toCell(CellStatus s) { return static_cast<Cell>(static_cast<int>(s)); }

struct ScanCell {
    Cell cell;
    int  y;
    int  x;
};

void printBoardWithOverlay(const GameBoard& board,
                           const std::vector<ScanCell>& overlay);

bool exists(const std::vector<ScanCell>& v, int x, int y);
void appendUnique(std::vector<ScanCell>& dst, const std::vector<ScanCell>& src);

// ActiveZone
std::vector<ScanCell> buildOverlayFromActiveZone(const ActiveZone& az);

#endif