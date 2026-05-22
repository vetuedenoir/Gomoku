#include "Gomoku.hpp"
#include "logger/Logger.hpp"

int main()
{
    Logger::info("MAIN", "Gomoku starting");

    Gomoku game;
    game.run();

    Logger::info("MAIN", "Gomoku exiting");
    return 0;
}
