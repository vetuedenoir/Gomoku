#ifndef PATTERNS_HPP
# define PATTERNS_HPP

#include "game/GameBoard.hpp"
#include "direction.hpp"
#include <vector>

// ── Cell ─────────────────────────────────────────────────────────────────────
//
// Extends CellStatus with an out-of-bounds sentinel used exclusively by line
// scanning. Numeric values match CellStatus so the cast is trivial:
//   Cell::Empty = CellStatus::Empty = 0
//   Cell::Black = CellStatus::Black = 1
//   Cell::White = CellStatus::White = 2
//   Cell::OOB   = 3   (no CellStatus equivalent)
//   Cell::OutOfBounds   = 3   (no CellStatus equivalent)

enum class Cell { Empty = 0, Black = 1, White = 2, OOB = 3, Scan = 4 };

inline Cell toCell(CellStatus s) { return static_cast<Cell>(static_cast<int>(s)); }

struct ScanCell {
    Cell cell;
    int  y;
    int  x;
};

// ── LineWindow ────────────────────────────────────────────────────────────────
//
// A fixed 9-cell snapshot of the board along one direction.
// center is always at index HALF (= 4).

// struct Window or Line ? 
// struct Line
struct LineWindow
{
    static constexpr int HALF = 2;
    static constexpr int SIZE = 2 * HALF + 1;  // 9

    Cell cells[SIZE];
    ScanCell scanCell[SIZE];

    Cell center()           const { return cells[HALF]; }
    Cell at(int offset)     const { return cells[HALF + offset]; }  // offset: -4..+4

    std::vector<ScanCell> toVector() const;
};


// ── scan_line ─────────────────────────────────────────────────────────────────
//
// Returns a 9-cell LineWindow centered on (col, row). One step = one intersection.? 
//
// virtualColor: color injected at the center cell, simulating a stone being placed there.
//               
//               Pass CellStatus::Empty to read the board as-is (no injection).
//
// Cells outside the board are Cell::OOB -> Cell::OutOfBounds
//
// Evaluating a placement of Black at (col, row):
//   scan_line(board, col, row, CellStatus::Black, Direction::E)
//
// Reading the board without any placement:
//   scan_line(board, col, row, CellStatus::Empty, Direction::E)

LineWindow scan_line(const GameBoard& board,
                     int col, int row,
                     CellStatus virtualColor,
                     Direction dir);
#endif // PATTERNS_HPP
