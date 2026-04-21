#ifndef OPENINGSCRIPT_HPP
# define OPENINGSCRIPT_HPP

#include "game/contracts/GameConfig.hpp"
#include "game/contracts/Move.hpp"
#include "game/GameBoard.hpp"
#include <vector>


// Identifies which player is responsible for a given step.
// "TentativeFirst"  = the player that initiated the opening (placed the first stones).
// "TentativeSecond" = the other player.
// Actual Black/White assignment is not known until ColorChoice resolves.
enum class OpeningActor
{
    TentativeFirst,
    TentativeSecond
};

struct PlacementConstraint
{
    bool mustBeCenter  = false;

    // Minimum Chebyshev distance from historyPlacedStones[refStoneIdx].
    // -1 = no distance constraint.
    int  refStoneIdx   = -1;
    int  minDist       = 0;
};

struct StoneSpec
{
    CellStatus           color;
    PlacementConstraint  constraint;
};

// ── One scripted step ────────────────────────────────────────────────────────

struct OpeningStep
{
    OpeningActor           actor;
    std::vector<StoneSpec> stones;                      // ordered sequence to place
    bool                   samePlayerPlacesAll = false; // one player places every stone in this step
    bool                   triggersColorChoice = false; // transition to ColorChoice when complete
};


struct GameState;

// ── API ─────────────────────────────────────────────────────────────

std::vector<OpeningStep> getOpeningScript(OpeningRule rule);

OpeningStep              getCurrentOpeningStep(const GameState& state);

bool                     isOpeningPlacementFinished(const GameState& state);

// Places one stone according to the current sub-step. 
// Advances step/sub-step counters and sets the next phase when a step completes.
// Returns false if the move violates any constraint or is otherwise illegal.
bool                     applyOpeningMove(GameState& state, const Move& move);

// TODO: Related to bitboard
// Returns every legal (col, row) cell as a Move with the script-forced color.
std::vector<Move>        generateOpeningMoves(const GameState& state);

#endif
