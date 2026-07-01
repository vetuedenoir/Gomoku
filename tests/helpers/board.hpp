#pragma once
#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include <initializer_list>
#include <string>

// '.' = empty, 'B' = Black, 'W' = White.
inline GameBoard boardFromAscii(std::initializer_list<const char*> rows, Color toMove = Color::Black)
{
    GameBoard b = empty_board();
    int y = 0;
    for (const char* row : rows) {
        for (int x = 0; row[x]; ++x) {
            char c = row[x];
            if (c == '.') place(b, x, y, CellStatus::Empty);
            else if (c == 'B') place(b, x, y, CellStatus::Black);
            else if (c == 'W') place(b, x, y, CellStatus::White);
        }
        ++y;
    }
    b.setCurrentColor(toMove);
    return b;
}