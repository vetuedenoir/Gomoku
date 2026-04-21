#ifndef GAMEPHASE_HPP
# define GAMEPHASE_HPP

enum class GamePhase
{
    OpeningPlacement,  // stones placed under rule-specific script; actor may change
    ColorChoice,       // tentative player decides which color to play as
    NormalPlay         // standard alternating play
};

#endif
