// DebugBoard.cpp

#include "game/DebugBoard.hpp"
#include <iostream>

static bool isOverlay(int x, int y, const std::vector<ScanCell>& overlay)
{
    for (std::vector<ScanCell>::const_iterator it = overlay.begin();
         it != overlay.end();
         ++it)
    {
        if (it->x == x && it->y == y)
            return true;
    }
    return false;
}

void printBoardWithOverlay(const GameBoard& board,
                           const std::vector<ScanCell>& overlay)
{
    int size = board.getSize();

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            CellStatus c = board.getCell(x, y);
            if (c == CellStatus::Black) std::cout << "B ";
            else if (c == CellStatus::White) std::cout << "W ";
            else {
                if (isOverlay(x, y, overlay))
                    std::cout << "* ";
                else std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
}

bool exists(const std::vector<ScanCell>& v, int x, int y)
{
    for (std::vector<ScanCell>::const_iterator it = v.begin(); it != v.end(); ++it)
    {
        if (it->x == x && it->y == y)
            return true;
    }
    return false;
}

void appendUnique(std::vector<ScanCell>& dst, const std::vector<ScanCell>& src)
{
    for (std::vector<ScanCell>::const_iterator it = src.begin(); it != src.end(); ++it)
    {
        if (!exists(dst, it->x, it->y))
            dst.push_back(*it);
    }
}

std::vector<ScanCell> buildOverlayFromActiveZone(const ActiveZone& az)
{
    std::vector<ScanCell> overlay;

    const bitboard19& mask = az.getCandidateMask();

    for (int y = 0; y < 19; y++)
    {
        for (int x = 0; x < 19; x++)
        {
            if (get_bb19(mask, index_bb19(x, y)))
            {
                ScanCell cell;
                cell.x = x;
                cell.y = y;
                overlay.push_back(cell);
            }
        }
    }

    return overlay;
}