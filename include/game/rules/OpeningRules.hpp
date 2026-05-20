#ifndef OPENINGRULES_HPP
# define OPENINGRULES_HPP

#include "game/contracts/GameConfig.hpp"
#include "game/contracts/Move.hpp"
#include "game/board/GameBoard.hpp"
#include <vector>

enum class OpeningActor
{
    TentativeFirst,
    TentativeSecond
};

struct PlacementConstraint
{
    bool mustBeCenter = false;
    int  refStoneIdx  = -1;
    int  minDist      = 0;
};

struct StoneSpec
{
    CellStatus          color;
    PlacementConstraint constraint;
};

struct OpeningStep
{
    OpeningActor           actor;
    std::vector<StoneSpec> stones;
    bool                   samePlayerPlacesAll = false;
    bool                   triggersColorChoice = false;
};

struct GameState;

std::vector<OpeningStep> buildOpeningSteps(OpeningProtocol openingProtocol);

OpeningStep              getCurrentOpeningStep(const GameState& state);

bool                     isOpeningComplete(const GameState& state);

bool                     canPlaceOpeningStone(const GameState& state, const Move& move);

bool                     commitOpeningMove(GameState& state, const Move& move);

std::vector<Move>        getLegalOpeningMoves(const GameState& state);

#endif
