#include "game/OpeningScript.hpp"
#include "game/GameState.hpp"
#include "game/Seat.hpp"
#include "logger/Logger.hpp"
#include <cmath>
#include <algorithm>
#include <string>

// ── Helpers for logging ─────────────────────────────────────────────────────────────

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
        case GamePhase::OpeningPlacement: return "Opening placement";
        case GamePhase::ColorChoice:      return "Color choice";
        case GamePhase::NormalPlay:       return "Normal play";
        default:                         return "Unknown phase";
    }
}

static Seat toSeat(OpeningActor a)
{
    return (a == OpeningActor::TentativeFirst) ? Seat::First : Seat::Second;
}

static std::string stepCtx(const GameState& state)
{
    const OpeningStep& step = state.openingScript[state.stepIdx];
    return "step " + std::to_string(state.stepIdx)
        + "/" + std::to_string(static_cast<int>(state.openingScript.size()) - 1)
        + "  sub "  + std::to_string(state.subIdx)
        + "/" + std::to_string(static_cast<int>(step.stones.size()) - 1)
        + "  actor=" + seatStr(toSeat(step.actor));
}

// ── Constraints ─────────────────────────────────────────────────────────────

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

// Builds one scripted step.
// samePlayerPlacesAll is always true for every rule currently defined; triggersColorChoice defaults to false.
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

// Pro and LongPro share the same 3-step structure; only the minimum distance
// for the third stone differs.
static std::vector<OpeningStep> proScript(int thirdStoneMinDist)
{
    return {
        makeStep(OpeningActor::TentativeFirst,  {{ CellStatus::Black, centerConstraint() }}),
        makeStep(OpeningActor::TentativeSecond, {{ CellStatus::White, {} }}),
        makeStep(OpeningActor::TentativeFirst,  {{ CellStatus::Black, distanceConstraint(0, thirdStoneMinDist) }}),
    };
}

// ── Script builder ────────────────────────────────────────────────────────────

std::vector<OpeningStep> getOpeningScript(OpeningRule rule)
{
    switch (rule)
    {
        case OpeningRule::Normal:
            return {};

        case OpeningRule::Pro:
            return proScript(3);

        case OpeningRule::LongPro:
            return proScript(4);

        case OpeningRule::Swap:
            return {
                makeStep(OpeningActor::TentativeFirst, {
                    { CellStatus::Black, {} },
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, /*triggersChoice=*/true),
            };

        case OpeningRule::Swap2:
            return {
                // Step 0 — TentativeFirst places B B W; TentativeSecond then chooses.
                makeStep(OpeningActor::TentativeFirst, {
                    { CellStatus::Black, {} },
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, /*triggersChoice=*/true),
                // Step 1 — only reached via Swap2 option 3: TentativeSecond places B W.
                makeStep(OpeningActor::TentativeSecond, {
                    { CellStatus::Black, {} },
                    { CellStatus::White, {} },
                }, /*triggersChoice=*/true),
            };
    }
    return {};
}

static int chebyshevDist(int c1, int r1, int c2, int r2)
{
    return std::max(std::abs(c1 - c2), std::abs(r1 - r2));
}

// Returns true when a cell satisfies the stone's placement constraint.
static bool isLegalPlacement(int col, int row, int boardSize,
                             const PlacementConstraint& c,
                             const std::vector<PlacedStone>& history)
{
    if (c.mustBeCenter)
    {
        int center = boardSize / 2;
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
    return state.openingScript[state.stepIdx];
}

bool isOpeningPlacementFinished(const GameState& state)
{
    return state.openingScript.empty()
        || state.stepIdx >= (int)state.openingScript.size();
}


static std::string openingStepDescription(OpeningRule rule, int stepIdx)
{
    switch (rule)
    {
        case OpeningRule::Pro:
            switch (stepIdx)
            {
                case 0: return "Pro  step 0/2 — TentativeFirst places 1 Black stone at the board centre (forced)";
                case 1: return "Pro  step 1/2 — TentativeSecond places 1 White stone anywhere on the board";
                case 2: return "Pro  step 2/2 — TentativeFirst places 1 Black stone "
                            "at least 3 intersections (Chebyshev) from the first stone";
            }
            break;

        case OpeningRule::LongPro:
            switch (stepIdx)
            {
                case 0: return "LongPro  step 0/2 — TentativeFirst places 1 Black stone at the board centre (forced)";
                case 1: return "LongPro  step 1/2 — TentativeSecond places 1 White stone anywhere on the board";
                case 2: return "LongPro  step 2/2 — TentativeFirst places 1 Black stone "
                            "at least 4 intersections (Chebyshev) from the first stone";
            }
            break;

        case OpeningRule::Swap:
            if (stepIdx == 0)
                return "Swap  step 0/0 — TentativeFirst places 3 stones (Black, Black, White) "
                    "anywhere; TentativeSecond will then choose which colour to play as";
            break;

        case OpeningRule::Swap2:
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

bool applyOpeningMove(GameState& state, const Move& move)
{
    if (isOpeningPlacementFinished(state))
    {
        Logger::warn("OPENING", "applyOpeningMove called but placement is already finished");
        return false;
    }

    const OpeningStep& step = state.openingScript[state.stepIdx];
    const StoneSpec&   spec = step.stones[state.subIdx];

    if (state.subIdx == 0)
        Logger::info("OPENING", openingStepDescription(state.openingRule, state.stepIdx));

    const std::string stepInfo = stepCtx(state);

    // TODO: AI engine protection
    //  Reject wrong forced color
    // if (move.forcedColor != spec.color)
    // {
    //     Logger::warn("OPENING",
    //         stepInfo + " | wrong colour: got " + colorStr(move.forcedColor)
    //         + ", expected " + colorStr(spec.color));
    //     return false;
    // }

    if (!isLegalPlacement(move.col, move.row, state.board.getSize(),
                         spec.constraint, state.historyPlacedStones))
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
        return false;
    }

    if (!state.board.placeStoneOfColor(move.col, move.row, spec.color))
    {
        Logger::warn("OPENING",
            stepInfo + " | (" + std::to_string(move.col) + "," + std::to_string(move.row)
            + ") cell occupied");
        return false;
    }

    Logger::debug("OPENING",
        stepInfo + " | " + colorStr(spec.color)
        + " → (" + std::to_string(move.col) + "," + std::to_string(move.row) + ") ✓");

    state.historyPlacedStones.push_back({ move.col, move.row, spec.color });

    ++state.subIdx;

    if (state.subIdx < (int)step.stones.size())
        return true;  // more stones to place in this step

    const bool hadColorChoice = step.triggersColorChoice;
    ++state.stepIdx;
    state.subIdx = 0;

    Logger::debug("OPENING",
        "step complete — triggersColorChoice=" + std::string(hadColorChoice ? "true" : "false"));

    if (hadColorChoice)
    {
        const GamePhase prev    = state.phase;
        state.phase       = GamePhase::ColorChoice;
        state.currentActor = otherSeat(toSeat(step.actor));

        Logger::info("PHASE",
            std::string(phaseStr(prev)) + " → " + phaseStr(state.phase)
            + "  chooser=" + seatStr(state.currentActor));
    }
    else if (state.stepIdx >= (int)state.openingScript.size())
    {
        const GamePhase prev = state.phase;
        state.phase      = GamePhase::NormalPlay;
        state.blackSeat  = Seat::First;
        state.whiteSeat  = Seat::Second;
        state.board.setCurrentPlayer(Seat::First);

        Logger::info("PHASE",
            std::string(phaseStr(prev)) + " → " + phaseStr(state.phase)
            + "  (opening complete — Black=" + seatStr(Seat::First)
            + ", White=" + seatStr(Seat::Second) + ")");
    }

    return true;
}

std::vector<Move> generateOpeningMoves(const GameState& state)
{
    (void)state;
    return {};
}