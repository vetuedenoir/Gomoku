#include "game/GameState.hpp"
#include "game/board/GameBoard15.hpp"
#include "game/board/GameBoard19.hpp"
#include "logger/Logger.hpp"
#include <string>

// ── Logging helpers  ───────────────────────────────────────────────

static const char* phaseStr(GamePhase p)
{
    switch (p)
    {
        case GamePhase::OpeningPlacement: return "OpeningPlacement";
        case GamePhase::ColorChoice:      return "ColorChoice";
        case GamePhase::NormalPlay:       return "NormalPlay";
    }
    return "Unknown";
}

static const char* ruleStr(OpeningRule r)
{
    switch (r)
    {
        case OpeningRule::Normal:  return "Normal";
        case OpeningRule::Pro:     return "Pro";
        case OpeningRule::LongPro: return "LongPro";
        case OpeningRule::Swap:    return "Swap";
        case OpeningRule::Swap2:   return "Swap2";
    }
    return "Unknown";
}

GameState::GameState(int boardSize, OpeningRule rule, StoneColor firstPlayer)
    : phase(GamePhase::OpeningPlacement),
      openingRule(rule),
      stepIdx(0),
      subIdx(0),
      currentActor(Seat::First)
{
    // instantiate the board wrapper which will create the correct sized implementation
    board = std::make_unique<GameBoard>(boardSize, (firstPlayer == StoneColor::Black) ? Seat::First : Seat::Second);
    openingSteps = buildOpeningSteps(rule);

    if (openingSteps.empty())
    {
        phase     = GamePhase::NormalPlay;
        blackSeat = (firstPlayer == StoneColor::Black) ? Seat::First : Seat::Second;
        whiteSeat = (firstPlayer == StoneColor::Black) ? Seat::Second : Seat::First;
    }

    Logger::info("GAMESTATE",
        std::string("rule=") + ruleStr(rule)
        + "  board=" + std::to_string(boardSize) + "x" + std::to_string(boardSize)
        + "  phase=" + phaseStr(phase)
        + "  steps=" + std::to_string(openingSteps.size()));
}


void GameState::resolveColorChoice(bool swapped)
{
    Logger::info("CHOICE",
        seatStr(currentActor)
        + (swapped ? " swapped → takes opposite colour"
                   : " keeps default colour"));

    // Default: Seat::First → Black, Seat::Second → White.
    if (!swapped)
    {
        blackSeat = Seat::First;
        whiteSeat = Seat::Second;
    }
    else
    {
        blackSeat = Seat::Second;
        whiteSeat = Seat::First;
    }

    Logger::info("RESOLVE",
        "Black → " + seatStr(*blackSeat)
        + "  |  White → " + seatStr(*whiteSeat));

    const GamePhase prev = phase;
    phase = GamePhase::NormalPlay;
    board->setCurrentPlayer(Seat::First);  // Black (Seat::First) always opens normal play

    Logger::info("PHASE",
        std::string(phaseStr(prev)) + " → " + phaseStr(phase));
}

// ── Swap2 option 3 ────────────────────────────────────────────────────────────

void GameState::continueOpeningPlacement()
{
    Logger::info("CHOICE",
        "Seat::Second chose option 3 — placing 2 more stones (B + W)");

    const GamePhase prev = phase;
    phase        = GamePhase::OpeningPlacement;
    currentActor = Seat::Second;
    subIdx       = 0;

    Logger::info("PHASE",
        std::string(phaseStr(prev)) + " → " + phaseStr(phase)
        + "  actor=" + seatStr(currentActor)
        + "  step=" + std::to_string(stepIdx));
}

CellStatus GameState::nextOpeningColor() const
{
    if (isOpeningComplete(*this))
        return CellStatus::Empty;
    return openingSteps[stepIdx].stones[subIdx].color;
}
