#ifndef OPENINGRUNTIME_HPP
# define OPENINGRUNTIME_HPP

#include "game/contracts/contracts.hpp"
#include "game/validation/rules/OpeningRules.hpp"
#include <vector>

struct PlacedStone
{
    int        col;
    int        row;
    CellStatus color;
};

struct OpeningRuntime
{
    OpeningProtocol             openingProtocol;
    std::vector<OpeningStep>    openingSteps;
    int                         stepIdx = 0;
    int                         subIdx  = 0;
    std::vector<PlacedStone>    historyPlacedStones;

    explicit OpeningRuntime(OpeningProtocol protocol);

    void continueOpeningPlacement();
    CellStatus nextOpeningColor() const;
};

#endif
