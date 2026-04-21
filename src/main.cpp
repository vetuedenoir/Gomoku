#include "Gomoku.hpp"
#include "logger/Logger.hpp"
#include "optimization/ZobristHasher.hpp"

int main()
{
    // TODO: move into Engine or SearchPosition::init() when AI layer exists
    ZobristHasher(19).init();

    Logger::info("MAIN", "Gomoku starting");

    Gomoku game;
    game.run();

    Logger::info("MAIN", "Gomoku exiting");
    return 0;
}
