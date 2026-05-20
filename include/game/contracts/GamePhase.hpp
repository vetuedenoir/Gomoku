#ifndef GAMEPHASE_HPP
# define GAMEPHASE_HPP

enum class GamePhase
{
    Opening,           // stones placed under rule-specific script; actor may change
    ColorChoice,       // tentative player decides which color to play as
    Standard,            // standard alternating play
};

#endif
