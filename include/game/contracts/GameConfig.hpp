#ifndef GAMECONFIG_HPP
# define GAMECONFIG_HPP

#include "game/contracts/OpeningProtocol.hpp"
#include "game/contracts/Stone.hpp"


struct GameConfig
{
    int             boardSize   = 19;
    StoneColor      playerColor = StoneColor::Black;
    OpeningProtocol openingProtocol = OpeningProtocol::Standard;
};

#endif
