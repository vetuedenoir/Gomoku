#pragma once

#include "direction.hpp"
#include "game/board/GameBoard.hpp"
#include "ai/ActiveZone.hpp"
#include <vector>

inline t_cell cellAt(int x0, int y0, Direction dir, int step)
{
	return { x0 + step * dx(dir), y0 + step * dy(dir) };
}

inline void placeRun(GameBoard& b, int x0, int y0, Direction dir, int count, CellStatus color)
{
	for (int i = 0; i < count; ++i)
	{
		const t_cell c = cellAt(x0, y0, dir, i);
		b.placeStoneOfColor(c.x, c.y, color);
	}
}

// Cells immediately before the run and immediately after the last stone.
inline std::vector<t_cell> runEndpoints(int x0, int y0, Direction dir, int runLength)
{
	return {
		cellAt(x0, y0, dir, -1),
		cellAt(x0, y0, dir, runLength),
	};
}

inline bool isOneOf(int x, int y, const std::vector<t_cell>& cells)
{
	for (const t_cell& c : cells)
		if (c.x == x && c.y == y)
			return true;
	return false;
}

inline bool moveIsOneOf(const t_cell& move, const std::vector<t_cell>& cells)
{
	return isOneOf(move.x, move.y, cells);
}

struct ThreatLine
{
	Direction  dir;
	int        x0;
	int        y0;
	int        runLength;
	CellStatus attacker;
	CellStatus blocker;
	int        blockedEnd; // -1 = back, +1 = front, 0 = open both ends
};

inline void buildThreatLine(GameBoard& b, const ThreatLine& line)
{
	placeRun(b, line.x0, line.y0, line.dir, line.runLength, line.attacker);
	if (line.blockedEnd != 0)
	{
		const int    endStep = (line.blockedEnd < 0) ? -1 : line.runLength;
		const t_cell c       = cellAt(line.x0, line.y0, line.dir, endStep);
		b.placeStoneOfColor(c.x, c.y, line.blocker);
	}
}

inline std::vector<t_cell> blockCells(const ThreatLine& line)
{
	std::vector<t_cell> ends = runEndpoints(line.x0, line.y0, line.dir, line.runLength);
	if (line.blockedEnd < 0)
		ends.erase(ends.begin());
	else if (line.blockedEnd > 0)
		ends.pop_back();
	return ends;
}

inline std::vector<t_cell> winningCells(const ThreatLine& line)
{
	if (line.runLength != 4)
		return {};
	return blockCells(line);
}

inline std::vector<t_cell> extendToFourCells(const ThreatLine& line)
{
	return blockCells(line);
}
