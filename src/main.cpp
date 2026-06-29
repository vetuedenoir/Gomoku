#include "Gomoku.hpp"
#include "logger/Logger.hpp"

int main()
{
    LOG_INFO("MAIN", "Gomoku starting");

    Gomoku game;
    game.run();

    LOG_INFO("MAIN", "Gomoku exiting");
    return 0;
}
