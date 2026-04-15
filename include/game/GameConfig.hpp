#ifndef GAMECONFIG_HPP
# define GAMECONFIG_HPP

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
    int         boardSize   = 19;             // 15 or 19
    int         playerStone = 0;              // 0 = black, 1 = white  (random resolves immediately on click)
    OpeningRule openingRule = OpeningRule::Normal;
};

#endif
