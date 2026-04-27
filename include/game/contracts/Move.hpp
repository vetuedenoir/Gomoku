#ifndef MOVE_HPP
# define MOVE_HPP

#include "game/GameBoard.hpp"

// A move in the game.
// During OpeningPlacement, forcedColor is set by the opening script;
// During NormalPlay, forcedColor == CellStatus::Empty (use current player).
struct Move
{
    int        col;
    int        row;
    CellStatus forcedColor;

    bool operator==(const Move& other) const noexcept
    {
        return col == other.col && row == other.row;
    }
};

#endif
