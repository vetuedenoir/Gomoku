#include "doctest.h"
#include <chrono>
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/line_helpers.hpp"

namespace {

struct Timed { long long ms; t_cell move; SearchStats stats; };

Timed timeBestMove(MasterAI19& ai, SearchPosition19& pos, Color c)
{
    auto start = std::chrono::steady_clock::now();
    t_cell move = ai.findBestMove(pos, c);
    auto end = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return { ms, move, ai.lastSearchStats() };
}

}

TEST_CASE("[PERF] findBestMove timing by depth on empty board")
{
    for (int depth : {1, 2, 4, 6, 8}) {
        GameBoard b = empty_board();
        b.setCurrentColor(Color::Black);
        SearchPosition19 pos = SearchPosition19::fromBoard(b);

        MasterAI19 ai(depth, 1, Color::Black);

        Timed t = timeBestMove(ai, pos, Color::Black);

        double nps = t.ms > 0 ? (t.stats.nodesVisited * 1000.0 / t.ms) : 0.0;
        LOG_INFO("PERF",
            "depth=" + std::to_string(depth) +
            " time=" + std::to_string(t.ms) + "ms" +
            " nodes=" + std::to_string(t.stats.nodesVisited) +
            " nps=" + std::to_string((long long)nps));

        
        CHECK(t.stats.nodesVisited > 0);
        CHECK(t.ms < 500); 
    }
}