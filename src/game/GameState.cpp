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
        case GamePhase::Opening: return "Opening";
        case GamePhase::ColorChoice:      return "ColorChoice";
        case GamePhase::Standard:       return "Standard";
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
        default: return "Standard";
    }
}

GameState::GameState(int boardSize, OpeningProtocol openingProtocol, const Color playerColor)
    : phase(GamePhase::Opening),
      openingProtocol(openingProtocol),
      stepIdx(0),
      subIdx(0),
      currentActor(Seat::First)
{
    // instantiate the board wrapper which will create the correct sized implementation
    board = std::make_unique<GameBoard>(boardSize, (playerColor == Color::Black) ? Seat::First : Seat::Second);
    openingSteps = buildOpeningSteps(openingProtocol);

    if (openingSteps.empty())
    {
        phase     = GamePhase::Standard;
        blackSeat = (playerColor == Color::Black) ? Seat::First : Seat::Second;
        whiteSeat = (playerColor == Color::Black) ? Seat::Second : Seat::First;
    }

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
    phase = GamePhase::Standard;

    Logger::info("PHASE",
        std::string(phaseStr(prev)) + " → " + phaseStr(phase));
}

// ── Swap2 option 3 ────────────────────────────────────────────────────────────

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
