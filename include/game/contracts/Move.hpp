#ifndef MOVE_HPP
# define MOVE_HPP

#include "game/GameBoard.hpp"

// A move in the game.
// During OpeningPlacement, forced_color is set by the opening script;
// During NormalPlay, forced_color == CellStatus::Empty (use current player).
struct Move
{
    int        col;
    int        row;
    CellStatus forced_color;
};

#endif
