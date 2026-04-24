// DebugBoard.hpp
#ifndef DEBUGBOARD_HPP
#define DEBUGBOARD_HPP

#include "game/GameBoard.hpp"
#include "ai/ActiveZone.hpp"
#include "game/patterns.hpp"
#include <vector>

void printBoardWithOverlay(const GameBoard& board,
                           const std::vector<ScanCell>& overlay);

bool exists(const std::vector<ScanCell>& v, int x, int y);
void appendUnique(std::vector<ScanCell>& dst, const std::vector<ScanCell>& src);

// ActiveZone
std::vector<ScanCell> buildOverlayFromActiveZone(const ActiveZone& az);

#endif