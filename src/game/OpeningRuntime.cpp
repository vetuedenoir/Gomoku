#include "game/OpeningRuntime.hpp"
#include "logger/Logger.hpp"
#include <string>

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

OpeningRuntime::OpeningRuntime(OpeningProtocol protocol)
    : openingProtocol(protocol)
{
    openingSteps = buildOpeningSteps(protocol);

    Logger::info("OPENING",
        std::string("protocol=") + openingProtocolStr(protocol)
        + "  steps=" + std::to_string(openingSteps.size()));
}

void OpeningRuntime::continueOpeningPlacement()
{
    Logger::info("CHOICE",
        "Seat::Second chose option 3 — placing 2 more stones (B + W)");
    subIdx = 0;
}

CellStatus OpeningRuntime::nextOpeningColor() const
{
    if (isOpeningComplete(*this))
        return CellStatus::Empty;
    return openingSteps[stepIdx].stones[subIdx].color;
}
