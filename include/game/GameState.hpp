#ifndef GAMESTATE_HPP
# define GAMESTATE_HPP

#include "game/contracts/contracts.hpp"
# include "game/board/GameBoard.hpp"
#include "game/validation/rules/OpeningRules.hpp"
#include <vector>
#include <optional>

struct PlacedStone
{
    int        col;
    int        row;
    CellStatus color;
};

struct GameState
{
    std::unique_ptr<GameBoard>  board;
    GamePhase                   phase;
    OpeningProtocol             openingProtocol;

    std::vector<OpeningStep> openingSteps;
    int                      stepIdx;
    int                      subIdx;

    std::vector<PlacedStone> historyPlacedStones;

    // Which seat is currently expected to act (place a stone or make a colour decision).
    Seat currentActor;

    GameState(int boardSize, OpeningProtocol openingProtocol);

    // Transition to Standard after colour selection (assignment handled by GameController).
    void resolveColorChoice(bool swapped);

    // Swap2 option 3: TentativeSecond places 2 more stones instead of choosing.
    void continueOpeningPlacement();
    
    CellStatus nextOpeningColor() const;
};

#endif
