#include "game/patterns.hpp"
#include <vector>

static LineWindow scan_line_impl(const GameBoard& board,
                                 int col, int row,
                                 CellStatus vcolor,
                                 int ddx, int ddy)
{
    LineWindow w;
    const int  size = board.getSize();

    for (int i = 0; i < LineWindow::SIZE; i++)
    {
        const int offset = i - LineWindow::HALF;  // -4 .. +4
        const int x      = col + offset * ddx;
        const int y      = row + offset * ddy;

        if (x < 0 || x >= size || y < 0 || y >= size) {
            w.cells[i] = Cell::OOB;
            w.scanCell[i] = {Cell::OOB, y, x};
        }
        else if (offset == 0 && vcolor != CellStatus::Empty) {
            w.cells[i] = toCell(vcolor);
            w.scanCell[i] = {toCell(vcolor), y, x};
        }
        else {
            Cell c = toCell(board.getCell(x, y));
            w.cells[i] = c;
            w.scanCell[i] = { Cell::Scan, y, x };
        }
    }
    return w;
}

LineWindow scan_line(const GameBoard& board,
                     int col, int row,
                     CellStatus vcolor,
                     Direction dir)
{
    return scan_line_impl(board, col, row, vcolor, dx(dir), dy(dir));
}

std::vector<ScanCell> LineWindow::toVector() const
{
    return std::vector<ScanCell>(scanCell, scanCell + SIZE);
}