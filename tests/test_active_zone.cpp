#include "doctest.h"
#include "game/patterns.hpp"
#include "game/GameBoard.hpp"
#include "ai/ActiveZone.hpp"
#include "game/DebugBoard.hpp"

// ── helpers ───────────────────────────────────────────────────────────────────

static GameBoard empty_board()
{
    return GameBoard(19, Seat::First);
}

static void place(GameBoard& b, int col, int row, CellStatus color)
{
    b.placeStoneOfColor(col, row, color);
}

TEST_CASE("scan_line: placed stones appear at correct window offsets")
{
    GameBoard b = empty_board();

    place(b, 1,  1, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);
    place(b, 11,  9, CellStatus::Black);
    place(b, 1, 16, CellStatus::Black);
    place(b, 18, 2, CellStatus::White);


    ActiveZone zone(2);
    
    zone.initialize(b);

    std::vector<ScanCell> overlay = buildOverlayFromActiveZone(zone);

    // appendUnique(b, v);
    printBoardWithOverlay(b, overlay);
}