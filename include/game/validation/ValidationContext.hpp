#ifndef VALIDATIONCONTEXT_HPP
# define VALIDATIONCONTEXT_HPP

#include "game/OpeningRuntime.hpp"
#include "game/board/GameBoard.hpp"

struct ValidationContext
{
    const GameBoard&      board;
    const OpeningRuntime& opening;
};

#endif
