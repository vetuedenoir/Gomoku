#ifndef PATTERNS_HPP
# define PATTERNS_HPP

#include "game/GameBoard.hpp"
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

// ── Direction ─────────────────────────────────────────────────────────────────
//
// Eight compass directions ordered clockwise from East.
// The underlying int value indexes into DIR_DX / DIR_DY.

enum class Direction : int { E = 0, SE = 1, S = 2, SW = 3, W = 4, NW = 5, N = 6, NE = 7 };

constexpr int DIR_DX[8] = {  1,  1,  0, -1, -1, -1,  0,  1 };
constexpr int DIR_DY[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };

inline int dx(Direction d) { return DIR_DX[static_cast<int>(d)]; }
inline int dy(Direction d) { return DIR_DY[static_cast<int>(d)]; }

// The 4 canonical directions for full-line scanning.
// Each direction implicitly covers its opposite through the window's negative offsets:
//   E covers E–W, S covers N–S, SE covers NW–SE, NE covers SW–NE.
constexpr Direction LINE_DIRS[4] = { Direction::E, Direction::SE, Direction::S, Direction::NE };


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
