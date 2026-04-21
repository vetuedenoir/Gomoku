#ifndef GAMECONFIG_HPP
# define GAMECONFIG_HPP

enum class StoneColor
{
    Black = 0,
    White = 1
};

enum class OpeningRule
{
    Normal  = 0,
    Pro     = 1,
    LongPro = 2,
    Swap    = 3,
    Swap2   = 4
};

struct GameConfig
{
    int         boardSize   = 19;
    StoneColor  playerStoneColor = StoneColor::Black;
    OpeningRule openingRule = OpeningRule::Normal;
};

#endif
