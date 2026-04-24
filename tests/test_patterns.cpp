#include "doctest.h"
#include "game/patterns.hpp"
#include "game/GameBoard.hpp"
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

    //   col: 7  8  9 10 11
    //        .  B  .  W  .    (row 9, scanning East)
    place(b, 8,  9, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);

    b.printGameBoard();

    LineWindow w = scan_line(b, 9, 9, CellStatus::Empty, Direction::E);
    LineWindow l1 = scan_line(b, 9, 9, CellStatus::Empty, Direction::SE);
    LineWindow l2 = scan_line(b, 9, 9, CellStatus::Empty, Direction::S);
    LineWindow l3 = scan_line(b, 9, 9, CellStatus::Empty, Direction::NE);


    CHECK(w.at(-1) == Cell::Black);
    CHECK(w.at( 0) == Cell::Empty);
    CHECK(w.at(+1) == Cell::White);


    std::vector<ScanCell> v = w.toVector();
    std::vector<ScanCell> v1 = l1.toVector();
    std::vector<ScanCell> v2 = l2.toVector();
    std::vector<ScanCell> v3 = l3.toVector();

    appendUnique(v, v1);
    appendUnique(v, v2);
    appendUnique(v, v3);

    printBoardWithOverlay(b, v);
}


// ── Direction enum ────────────────────────────────────────────────────────────

TEST_CASE("Direction: dx/dy accessors match expected offsets")
{
    CHECK(dx(Direction::E)  ==  1);  CHECK(dy(Direction::E)  ==  0);
    CHECK(dx(Direction::SE) ==  1);  CHECK(dy(Direction::SE) ==  1);
    CHECK(dx(Direction::S)  ==  0);  CHECK(dy(Direction::S)  ==  1);
    CHECK(dx(Direction::SW) == -1);  CHECK(dy(Direction::SW) ==  1);
    CHECK(dx(Direction::W)  == -1);  CHECK(dy(Direction::W)  ==  0);
    CHECK(dx(Direction::NW) == -1);  CHECK(dy(Direction::NW) == -1);
    CHECK(dx(Direction::N)  ==  0);  CHECK(dy(Direction::N)  == -1);
    CHECK(dx(Direction::NE) ==  1);  CHECK(dy(Direction::NE) == -1);
}

TEST_CASE("Direction: LINE_DIRS covers all four line orientations")
{
    CHECK(LINE_DIRS[0] == Direction::E);
    CHECK(LINE_DIRS[1] == Direction::SE);
    CHECK(LINE_DIRS[2] == Direction::S);
    CHECK(LINE_DIRS[3] == Direction::NE);
}

// ── scan_line ─────────────────────────────────────────────────────────────────

TEST_CASE("scan_line: empty board, no injection — all cells are Empty")
{
    GameBoard b = empty_board();

    LineWindow w = scan_line(b, 9, 9, CellStatus::Empty, Direction::E);

    for (int i = 0; i < LineWindow::SIZE; i++)
        CHECK(w.cells[i] == Cell::Empty);
}

TEST_CASE("scan_line: placed stones appear at correct window offsets")
{
    GameBoard b = empty_board();

    //   col: 7  8  9 10 11
    //        .  B  .  W  .    (row 9, scanning East)
    place(b, 8,  9, CellStatus::Black);
    place(b, 10, 9, CellStatus::White);

    LineWindow w = scan_line(b, 9, 9, CellStatus::Empty, Direction::E);

    CHECK(w.at(-1) == Cell::Black);
    CHECK(w.at( 0) == Cell::Empty);
    CHECK(w.at(+1) == Cell::White);
}

TEST_CASE("scan_line: vcolor injects a stone at the center")
{
    GameBoard b = empty_board();

    // Board has nothing at (9,9); vcolor=Black makes the center read as Black.
    LineWindow w = scan_line(b, 9, 9, CellStatus::Black, Direction::E);

    CHECK(w.center() == Cell::Black);
    CHECK(w.at(+1)   == Cell::Empty);  // surrounding cells unaffected
    CHECK(w.at(-1)   == Cell::Empty);
}

TEST_CASE("scan_line: vcolor does not affect cells other than the center")
{
    GameBoard b = empty_board();
    place(b, 8, 9, CellStatus::Black);

    // Virtual White at center (9,9) — real Black at (8,9) must still read as Black
    LineWindow w = scan_line(b, 9, 9, CellStatus::White, Direction::E);

    CHECK(w.center() == Cell::White);  // injected
    CHECK(w.at(-1)   == Cell::Black);  // real stone, unchanged
    CHECK(w.at(+1)   == Cell::Empty);
}

TEST_CASE("scan_line: cells beyond the right board edge are OOB")
{
    GameBoard b = empty_board();

    // Scan East from (17, 0) — col 19 is out of bounds (board is 0..18)
    LineWindow w = scan_line(b, 17, 0, CellStatus::Empty, Direction::E);

    CHECK(w.at(+1) == Cell::Empty);  // col 18 — last valid column
    CHECK(w.at(+2) == Cell::OOB);    // col 19 — out of bounds
    CHECK(w.at(+3) == Cell::OOB);
    CHECK(w.at(+4) == Cell::OOB);
}

TEST_CASE("scan_line: cells beyond the left board edge are OOB")
{
    GameBoard b = empty_board();

    // Scan East from (1, 0) — cols -3, -2, -1 are OOB at offsets -4, -3, -2
    LineWindow w = scan_line(b, 1, 0, CellStatus::Empty, Direction::E);

    CHECK(w.at(-4) == Cell::OOB);
    CHECK(w.at(-3) == Cell::OOB);
    CHECK(w.at(-2) == Cell::OOB);
    CHECK(w.at(-1) == Cell::Empty);  // col 0 — valid
    CHECK(w.at( 0) == Cell::Empty);  // col 1 — center
}

TEST_CASE("scan_line: Direction::N — positive offset moves up (decreasing row)")
{
    GameBoard b = empty_board();

    // at(+2) → (5,6), at(+1) → (5,7), center → (5,8), at(-1) → (5,9)
    place(b, 5, 6, CellStatus::Black);

    LineWindow w = scan_line(b, 5, 8, CellStatus::Empty, Direction::N);

    CHECK(w.at(+2)   == Cell::Black);
    CHECK(w.at(+1)   == Cell::Empty);
    CHECK(w.center() == Cell::Empty);
    CHECK(w.at(-1)   == Cell::Empty);
}

TEST_CASE("scan_line: Direction::SE diagonal")
{
    GameBoard b = empty_board();

    place(b, 8,  8,  CellStatus::Black);
    place(b, 10, 10, CellStatus::White);

    LineWindow w = scan_line(b, 9, 9, CellStatus::Empty, Direction::SE);

    CHECK(w.at(-1)   == Cell::Black);
    CHECK(w.center() == Cell::Empty);
    CHECK(w.at(+1)   == Cell::White);
}
