#include "game/validation/rules/OpeningRules.hpp"
#include "game/GameState.hpp"
#include "game/contracts/contracts.hpp"
#include "logger/Logger.hpp"
#include <cmath>
#include <algorithm>

static const char* colorStr(CellStatus c)
{
    switch (c)
    {
        case CellStatus::Black: return "Black";
        case CellStatus::White: return "White";
        default:                return "Empty";
    }
}

static const char* phaseStr(GamePhase p)
{
    switch (p)
    {
        case GamePhase::Opening: return "Opening";
        case GamePhase::ColorChoice:      return "Color choice";
        case GamePhase::Standard:       return "Standard";
        default:                          return "Unknown phase";
    }
}

static Seat toSeat(OpeningActor a)
{
    return (a == OpeningActor::TentativeFirst) ? Seat::First : Seat::Second;
}

static std::string stepCtx(const GameState& state)
{
    const OpeningStep& step = state.openingSteps[state.stepIdx];
    return "step " + std::to_string(state.stepIdx)
        + "/" + std::to_string(static_cast<int>(state.openingSteps.size()) - 1)
        + "  sub " + std::to_string(state.subIdx)
        + "/" + std::to_string(static_cast<int>(step.stones.size()) - 1)
        + "  actor=" + seatStr(toSeat(step.actor));
}

static PlacementConstraint centerConstraint()
{
    PlacementConstraint c;
    c.mustBeCenter = true;
    return c;
}

static PlacementConstraint distanceConstraint(int refIdx, int minDist)
{
    PlacementConstraint c;
    c.refStoneIdx = refIdx;
    c.minDist     = minDist;
    return c;
}

static OpeningStep makeStep(OpeningActor                    actor,
                            std::initializer_list<StoneSpec> stones,
                            bool                             triggersChoice = false)
{
    OpeningStep s;
    s.actor               = actor;
    s.samePlayerPlacesAll = true;
    s.triggersColorChoice = triggersChoice;
    s.stones              = stones;
    return s;
}

static std::vector<OpeningStep> proScript(int thirdStoneMinDist)
{
    return {
        makeStep(OpeningActor::TentativeFirst,  {{ CellStatus::Black, centerConstraint() }}),
        makeStep(OpeningActor::TentativeSecond, {{ CellStatus::White, {} }}),
        makeStep(OpeningActor::TentativeFirst,  {{ CellStatus::Black, distanceConstraint(0, thirdStoneMinDist) }}),
        makeStep(OpeningActor::TentativeSecond, {{ CellStatus::White, {} }}),
    };
}

std::vector<OpeningStep> buildOpeningSteps(OpeningProtocol openingProtocol)
{
    switch (openingProtocol)
    {
        case OpeningProtocol::Standard:
            return {};
        case OpeningProtocol::Pro:
            return proScript(3);
        case OpeningProtocol::LongPro:
            return proScript(4);
        case OpeningProtocol::Swap:
            return {
                makeStep(OpeningActor::TentativeFirst, {
                    { CellStatus::Black, {} },
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, true),
            };
        case OpeningProtocol::Swap2:
            return {
                makeStep(OpeningActor::TentativeFirst, {
                    { CellStatus::Black, {} },
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, true),
                makeStep(OpeningActor::TentativeSecond, {
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, true),
            };
    }
    return {};
}

static int chebyshevDist(int c1, int r1, int c2, int r2)
{
    return std::max(std::abs(c1 - c2), std::abs(r1 - r2));
}

static bool isLegalPlacement(int col, int row, int boardSize,
                             const PlacementConstraint& c,
                             const std::vector<PlacedStone>& history)
{
    if (c.mustBeCenter)
    {
        const int center = boardSize / 2;
        if (col != center || row != center)
            return false;
    }

    if (c.refStoneIdx >= 0 && c.refStoneIdx < (int)history.size())
    {
        const PlacedStone& ref = history[c.refStoneIdx];
        if (chebyshevDist(col, row, ref.col, ref.row) < c.minDist)
            return false;
    }

    return true;
}

OpeningStep getCurrentOpeningStep(const GameState& state)
{
    return state.openingSteps[state.stepIdx];
}

bool isOpeningComplete(const GameState& state)
{
    return state.openingSteps.empty()
        || state.stepIdx >= (int)state.openingSteps.size();
}

bool canPlaceOpeningStone(const GameState& state, const Move& move)
{
    if (isOpeningComplete(state))
        return false;

    const OpeningStep& step = state.openingSteps[state.stepIdx];
    const StoneSpec&   spec = step.stones[state.subIdx];

    if (move.forcedColor != CellStatus::Empty && move.forcedColor != spec.color)
        return false;

    if (!state.board->isFree(move.col, move.row))
        return false;

    if (!isLegalPlacement(move.col, move.row, state.board->getSize(),
                          spec.constraint, state.historyPlacedStones))
        return false;

    return true;
}

static std::string openingStepDescription(OpeningProtocol openingProtocol, int stepIdx)
{
    switch (openingProtocol)
    {
        case OpeningProtocol::Pro:
            switch (stepIdx)
            {
                case 0: return "Pro  step 0/2 — TentativeFirst places 1 Black stone at the board centre (forced)";
                case 1: return "Pro  step 1/2 — TentativeSecond places 1 White stone anywhere on the board";
                case 2: return "Pro  step 2/2 — TentativeFirst places 1 Black stone "
                            "at least 3 intersections (Chebyshev) from the first stone";
            }
            break;
        case OpeningProtocol::LongPro:
            switch (stepIdx)
            {
                case 0: return "LongPro  step 0/2 — TentativeFirst places 1 Black stone at the board centre (forced)";
                case 1: return "LongPro  step 1/2 — TentativeSecond places 1 White stone anywhere on the board";
                case 2: return "LongPro  step 2/2 — TentativeFirst places 1 Black stone "
                            "at least 4 intersections (Chebyshev) from the first stone";
            }
            break;
        case OpeningProtocol::Swap:
            if (stepIdx == 0)
                return "Swap  step 0/0 — TentativeFirst places 3 stones (Black, Black, White) "
                       "anywhere; TentativeSecond will then choose which colour to play as";
            break;
        case OpeningProtocol::Swap2:
            switch (stepIdx)
            {
                case 0: return "Swap2  step 0/1 — TentativeFirst places 3 stones (Black, Black, White) anywhere; "
                            "TentativeSecond may (A) play White, (B) swap to Black, "
                            "or (C) place 2 more stones and pass the choice back";
                case 1: return "Swap2  step 1/1 — TentativeSecond places 2 stones (Black, White) anywhere "
                            "(option C was chosen); TentativeFirst will now choose which colour to play as";
            }
            break;
        default:
            break;
    }
    return "Unknown step";
}

bool commitOpeningMove(GameState& state, const Move& move)
{
    if (isOpeningComplete(state))
    {
        Logger::warn("OPENING", "commitOpeningMove called but opening is already complete");
        return false;
    }

    const OpeningStep& step = state.openingSteps[state.stepIdx];
    const StoneSpec&   spec = step.stones[state.subIdx];

    if (state.subIdx == 0)
        Logger::info("OPENING", openingStepDescription(state.openingProtocol, state.stepIdx));

    const std::string stepInfo = stepCtx(state);

    if (!canPlaceOpeningStone(state, move))
    {
        if (move.forcedColor != CellStatus::Empty && move.forcedColor != spec.color)
        {
            Logger::warn("OPENING",
                stepInfo + " | wrong colour: got " + colorStr(move.forcedColor)
                + ", expected " + colorStr(spec.color));
        }
        else if (!state.board->isFree(move.col, move.row))
        {
            Logger::warn("OPENING",
                stepInfo + " | (" + std::to_string(move.col) + "," + std::to_string(move.row)
                + ") cell occupied");
        }
        else
        {
            std::string reason;
            if (spec.constraint.mustBeCenter)
                reason = "must be centre";
            else if (spec.constraint.refStoneIdx >= 0)
                reason = "too close to stone["
                         + std::to_string(spec.constraint.refStoneIdx)
                         + "] (min dist " + std::to_string(spec.constraint.minDist) + ")";
            else
                reason = "constraint violated";

            Logger::warn("OPENING",
                stepInfo + " | (" + std::to_string(move.col) + "," + std::to_string(move.row)
                + ") rejected — " + reason);
        }
        return false;
    }

    if (!state.board->placeStoneOfColor(move.col, move.row, spec.color))
        return false;

    Logger::debug("OPENING",
        stepInfo + " | " + colorStr(spec.color)
        + " → (" + std::to_string(move.col) + "," + std::to_string(move.row) + ") ✓");

    state.historyPlacedStones.push_back({ move.col, move.row, spec.color });

    ++state.subIdx;

    if (state.subIdx < (int)step.stones.size())
        return true;

    const bool hadColorChoice = step.triggersColorChoice;
    ++state.stepIdx;
    state.subIdx = 0;

    Logger::debug("OPENING",
        "step complete — triggersColorChoice=" + std::string(hadColorChoice ? "true" : "false"));

    if (hadColorChoice)
    {
        const GamePhase prev = state.phase;
        state.phase        = GamePhase::ColorChoice;
        state.currentActor = otherSeat(toSeat(step.actor));

        Logger::info("PHASE",
            std::string(phaseStr(prev)) + " → " + phaseStr(state.phase)
            + "  chooser=" + seatStr(state.currentActor));
    }
    else if (state.stepIdx < (int)state.openingSteps.size())
    {
        state.currentActor = toSeat(state.openingSteps[state.stepIdx].actor);
        Logger::debug("OPENING",
            "next step actor → " + seatStr(state.currentActor));
    }
    else if (state.stepIdx >= (int)state.openingSteps.size())
    {
        const GamePhase prev = state.phase;
        state.phase = GamePhase::Standard;


        Logger::info("PHASE HERE",
            std::string(phaseStr(prev)) + " → " + phaseStr(state.phase)
            + "  (opening complete — colour assignment handled by GameController)");
    }

    return true;
}

std::vector<Move> getLegalOpeningMoves(const GameState& state)
{
    std::vector<Move> moves;

    const OpeningStep& step = state.openingSteps[state.stepIdx];
    const StoneSpec&   spec = step.stones[state.subIdx];
    const int          size = state.board->getSize();
    const int          centerPerimeter = size / 2;

    for (int row = centerPerimeter - 1; row < centerPerimeter + 1; ++row)
    {
        for (int col = centerPerimeter - 1; col < centerPerimeter + 1; ++col)
        {
            const Move m{ col, row, spec.color };
            if (canPlaceOpeningStone(state, m))
                moves.push_back(m);
        }
    }

    Logger::info("OPENING",
        "getLegalOpeningMoves: " + std::to_string(moves.size()) + " candidates");
    return moves;
}
