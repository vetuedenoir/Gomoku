#ifndef GAMESTATE_HPP
# define GAMESTATE_HPP

#include "game/contracts/GameConfig.hpp"
#include "game/contracts/GamePhase.hpp"
#include "game/GameBoard.hpp"
#include "game/OpeningScript.hpp"
#include "game/Seat.hpp"
#include <optional>
#include <vector>

struct PlacedStone
{
    int        col;
    int        row;
    CellStatus color;
};

struct GameState
{
    GameBoard   board;
    GamePhase   phase;
    OpeningRule openingRule;

    std::vector<OpeningStep> openingScript;
    int                      stepIdx;
    int                      subIdx;

    std::vector<PlacedStone> historyPlacedStones;

    // Which seat is currently expected to act (place a stone or make a colour decision).
    Seat currentActor;

    std::optional<Seat> blackSeat;
    std::optional<Seat> whiteSeat;

    GameState(int boardSize, OpeningRule rule, StoneColor firstPlayer);

    // Call when currentActor has made their colour selection.
    //   swapped = false → actor keeps the "default" colour
    //                     (Seat::First → Black, Seat::Second → White)
    //   swapped = true  → actor takes the opposite colour
    void resolveColorChoice(bool swapped);

    // Swap2 option 3: TentativeSecond places 2 more stones instead of choosing.
    void continueOpeningPlacement();
    
    CellStatus nextOpeningColor() const;
};

#endif
