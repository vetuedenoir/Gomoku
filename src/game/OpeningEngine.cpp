#include "game/OpeningEngine.hpp"
#include "game/contracts/contracts.hpp"
#include "logger/Logger.hpp"
#include "config/config.hpp"
#include <cmath>
#include <algorithm>
#include <string>

// ── file-local helpers ────────────────────────────────────────────────────────

static const char* colorStr(CellStatus c)
{
    switch (c)
    {
        case CellStatus::Black: return "Black";
        case CellStatus::White: return "White";
        default:                return "Empty";
    }
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

static OpeningStep makeStep(Seat                             actor,
                            std::initializer_list<StoneSpec> stones,
                            bool                             triggersChoice = false)
{
    OpeningStep s;
    s.actor               = actor;
    s.triggersColorChoice = triggersChoice;
    s.stones              = stones;
    return s;
}

static std::vector<OpeningStep> proScript(int thirdStoneMinDist)
{
    return {
        makeStep(Seat::First,  {{ CellStatus::Black, centerConstraint() }}),
        makeStep(Seat::Second, {{ CellStatus::White, {} }}),
        makeStep(Seat::First,  {{ CellStatus::Black, distanceConstraint(0, thirdStoneMinDist) }}),
    };
}

static std::vector<OpeningStep> buildOpeningSteps(OpeningProtocol openingProtocol)
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
                makeStep(Seat::First, {
                    { CellStatus::Black, {} },
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, true),
            };
        case OpeningProtocol::Swap2:
            return {
                makeStep(Seat::First, {
                    { CellStatus::Black, {} },
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, true),
                makeStep(Seat::Second, {
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
                             const std::vector<t_cell>& history)
{
    if (c.mustBeCenter)
    {
        const int center = boardSize / 2;
        if (col != center || row != center)
            return false;
    }

    if (c.refStoneIdx >= 0 && c.refStoneIdx < (int)history.size())
    {
        const t_cell& ref = history[c.refStoneIdx];
        if (chebyshevDist(col, row, ref.x, ref.y) < c.minDist)
            return false;
    }

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

// ── OpeningEngine ─────────────────────────────────────────────────────────────

OpeningEngine::OpeningEngine(OpeningProtocol protocol)
    : _protocol(protocol), _steps(buildOpeningSteps(protocol))
{
    LOG_INFO("OPENING",
        std::string("protocol set")
        + "  steps=" + std::to_string(_steps.size()));
    LOG_SUPPRESS(_steps.size());
}

bool OpeningEngine::isComplete() const
{
    return _steps.empty() || _stepIdx >= (int)_steps.size();
}

CellStatus OpeningEngine::nextColor() const
{
    if (isComplete())
        return CellStatus::Empty;
    return _steps[_stepIdx].stones[_subIdx].color;
}

OpeningStep OpeningEngine::currentStep() const
{
    return _steps[_stepIdx];
}

OpeningProtocol OpeningEngine::protocol() const
{
    return _protocol;
}

int OpeningEngine::stepIndex() const
{
    return _stepIdx;
}

bool OpeningEngine::canPlace(const GameBoard& board, const Move& move) const
{
    if (isComplete())
        return false;

    const OpeningStep& step = _steps[_stepIdx];
    const StoneSpec&   spec = step.stones[_subIdx];

    if (move.forcedColor != CellStatus::Empty && move.forcedColor != spec.color)
        return false;

    if (!board.isFree(move.col, move.row))
        return false;

    if (!isLegalPlacement(move.col, move.row, board.getSize(),
                          spec.constraint, _history))
        return false;

    return true;
}

OpeningCommitResult OpeningEngine::commit(GameBoard& board, const Move& move)
{
    if (isComplete())
    {
        LOG_WARN("OPENING", "commit called but opening is already complete");
        return {};
    }

    const OpeningStep& step = _steps[_stepIdx];
    const StoneSpec&   spec = step.stones[_subIdx];

    if (_subIdx == 0)
    {
        LOG_INFO("OPENING", openingStepDescription(_protocol, _stepIdx));
        LOG_SUPPRESS(openingStepDescription(_protocol, _stepIdx));
    }

    const std::string stepInfo = "step " + std::to_string(_stepIdx)
        + "/" + std::to_string(static_cast<int>(_steps.size()) - 1)
        + "  sub " + std::to_string(_subIdx)
        + "/" + std::to_string(static_cast<int>(step.stones.size()) - 1)
        + "  actor=" + seatStr(step.actor);

    if (!canPlace(board, move))
    {
        if (move.forcedColor != CellStatus::Empty && move.forcedColor != spec.color)
        {
            LOG_WARN("OPENING",
                stepInfo + " | wrong colour: got " + colorStr(move.forcedColor)
                + ", expected " + colorStr(spec.color));
            LOG_SUPPRESS(stepInfo, colorStr(move.forcedColor), colorStr(spec.color));
        }
        else
        {
            if (!board.isFree(move.col, move.row))
            {
                LOG_WARN("OPENING",
                    stepInfo + " | (" + std::to_string(move.col) + "," + std::to_string(move.row)
                    + ") cell occupied");
                LOG_SUPPRESS(stepInfo, move.col, move.row);
            }
            std::string reason;
            if (spec.constraint.mustBeCenter)
                reason = "must be centre";
            else if (spec.constraint.refStoneIdx >= 0)
                reason = "too close to stone["
                         + std::to_string(spec.constraint.refStoneIdx)
                         + "] (min dist " + std::to_string(spec.constraint.minDist) + ")";
            else
                reason = "constraint violated";

            LOG_WARN("OPENING",
                stepInfo + " | (" + std::to_string(move.col) + "," + std::to_string(move.row)
                + ") rejected — " + reason);
            LOG_SUPPRESS(stepInfo, move.col, move.row, reason);
        }
        return {};
    }

    if (!board.placeStoneOfColor(move.col, move.row, spec.color))
        return {};

    LOG_DEBUG("OPENING",
        stepInfo + " | " + colorStr(spec.color)
        + " → (" + std::to_string(move.col) + "," + std::to_string(move.row) + ") ✓");
    LOG_SUPPRESS(stepInfo, move.col, move.row, spec.color);

    _history.push_back({ move.col, move.row });

    ++_subIdx;

    if (_subIdx < (int)step.stones.size())
        return { true, OpeningEvent::StepContinues, std::nullopt };

    const bool hadColorChoice = step.triggersColorChoice;
    ++_stepIdx;
    _subIdx = 0;

    LOG_DEBUG("DEBUG OPENING RULES",
        "step complete — triggersColorChoice=" + std::string(hadColorChoice ? "true" : "false"));

    if (hadColorChoice)
    {
        const Seat chooser = otherSeat(step.actor);
        LOG_INFO("OPENING", "step complete — color choice for " + seatStr(chooser));
        return { true, OpeningEvent::ColorChoice, chooser };
    }

    if (_stepIdx < (int)_steps.size())
    {
        const Seat next = _steps[_stepIdx].actor;
        LOG_DEBUG("OPENING", "next step actor → " + seatStr(next));
        return { true, OpeningEvent::NextStep, next };
    }

    const Seat next = otherSeat(step.actor);
    LOG_INFO("OPENING", "opening complete — next seat " + seatStr(next));
    return { true, OpeningEvent::Finished, next };
}

std::vector<Move> OpeningEngine::legalMoves(const GameBoard& board) const
{
    std::vector<Move> moves;

    const OpeningStep& step = _steps[_stepIdx];
    const StoneSpec&   spec = step.stones[_subIdx];
    const int          size = board.getSize();
    const int          centerPerimeter = size / 2;

    for (int row = centerPerimeter - 4; row < centerPerimeter + 4; ++row)
    {
        for (int col = centerPerimeter - 4; col < centerPerimeter + 4; ++col)
        {
            const Move m{ col, row, spec.color };
            if (canPlace(board, m))
                moves.push_back(m);
        }
    }

    LOG_DEBUG("OPENING",
        "legalMoves: " + std::to_string(moves.size()) + " candidates");

    return moves;
}

void OpeningEngine::continuePlacement()
{
    LOG_INFO("CHOICE",
        "Seat::Second chose option 3 — placing 2 more stones (B + W)");
    _subIdx = 0;
}
