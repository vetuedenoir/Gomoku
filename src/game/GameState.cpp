#include "game/GameState.hpp"
#include "logger/Logger.hpp"
#include <string>

static const char* phaseStr(GamePhase p)
{
    switch (p)
    {
        case GamePhase::Opening:     return "Opening";
        case GamePhase::ColorChoice: return "ColorChoice";
        case GamePhase::Standard:    return "Standard";
    }
    return "Unknown";
}

static const char* openingProtocolStr(OpeningProtocol openingProtocol)
{
    switch (openingProtocol)
    {
        case OpeningProtocol::Pro:     return "Pro";
        case OpeningProtocol::LongPro: return "LongPro";
        case OpeningProtocol::Swap:    return "Swap";
        case OpeningProtocol::Swap2:   return "Swap2";
        default:                       return "Standard";
    }
}

GameState::GameState(int boardSize, OpeningProtocol openingProtocol)
    : phase(GamePhase::Opening),
      openingProtocol(openingProtocol),
      stepIdx(0),
      subIdx(0),
      currentActor(Seat::First)
{
    board = std::make_unique<GameBoard>(boardSize, Color::Black);
    openingSteps = buildOpeningSteps(openingProtocol);

    // todo: why to check ? 
    if (openingSteps.empty())
        phase = GamePhase::Standard;

    Logger::info("GAMESTATE",
        std::string("openingProtocol=") + openingProtocolStr(openingProtocol)
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

    const GamePhase prev = phase;
    phase = GamePhase::Standard;

    Logger::info("PHASE",
        std::string(phaseStr(prev)) + " → " + phaseStr(phase));
}

void GameState::continueOpeningPlacement()
{
    Logger::info("CHOICE",
        "Seat::Second chose option 3 — placing 2 more stones (B + W)");

    const GamePhase prev = phase;
    phase        = GamePhase::Opening;
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
