#ifndef OPENINGRULES_HPP
# define OPENINGRULES_HPP

#include "game/contracts/contracts.hpp"
#include "game/board/GameBoard.hpp"
#include <optional>
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

struct OpeningRuntime;
struct PlacedStone;

enum class OpeningEvent
{
    StepContinues,
    NextStep,
    ColorChoice,
    Finished,
};

struct OpeningCommitResult
{
    bool                success = false;
    OpeningEvent        event   = OpeningEvent::StepContinues;
    std::optional<Seat> nextSeat;
};

std::vector<OpeningStep> buildOpeningSteps(OpeningProtocol openingProtocol);

OpeningStep              getCurrentOpeningStep(const OpeningRuntime& runtime);

bool                     isOpeningComplete(const OpeningRuntime& runtime);

bool                     canPlaceOpeningStone(const OpeningRuntime& runtime,
                                              const GameBoard& board,
                                              const Move& move);

OpeningCommitResult      commitOpeningMove(OpeningRuntime& runtime,
                                           GameBoard& board,
                                           const Move& move);

std::vector<Move>        getLegalOpeningMoves(const OpeningRuntime& runtime,
                                              const GameBoard& board);

#endif
