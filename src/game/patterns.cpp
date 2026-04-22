#include "game/patterns.hpp"

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

        if (x < 0 || x >= size || y < 0 || y >= size)
            w.cells[i] = Cell::OOB;
        else if (offset == 0 && vcolor != CellStatus::Empty)
            w.cells[i] = toCell(vcolor);
        else
            w.cells[i] = toCell(board.getCell(x, y));
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
