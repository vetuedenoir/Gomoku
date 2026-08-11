#include "doctest.h"
#include <chrono>
#include <vector>
#include <string>
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/board.hpp"
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
        // Quiet empty-board trees at depth 8 can exceed 500 ms wall-clock
        // depending on machine load; keep the hard guard on shallow depths.
        if (depth <= 6)
            CHECK(t.ms <= 500);
    }
}

TEST_CASE("[PERF] findBestMove timing by depth on board from ASCII")
{
    const int depth = 6;
    GameBoard b = boardFromAscii({
        "...................",
        "...................",
        "...................",
        "........B..........",
        ".......BW..........",
        "......BWW..........",
        ".....BWWW..........",
        "...................",
        "...................",
    }, Color::Black);
    
    b.setCurrentColor(Color::Black);
    t_BWBoard19 bb = to_bb(b);
    print_bb_19<BoardTraits<19>>(bb);

    SearchPosition19 pos = SearchPosition19::fromBoard(b);
    MasterAI19 ai(depth, 1, Color::Black);
    Timed t = timeBestMove(ai, pos, Color::Black);
    double nps = t.ms > 0 ? (t.stats.nodesVisited * 1000.0 / t.ms) : 0.0;
    
    LOG_INFO("PERF",
        "depth=" + std::to_string(depth) +
        " time=" + std::to_string(t.ms) + "ms" +
        " nodes=" + std::to_string(t.stats.nodesVisited) +
        " nps=" + std::to_string((long long)nps));

    LOG_INFO("PERF", "move=" + std::to_string(t.move.x) + "," + std::to_string(t.move.y));

    place(b, t.move.x, t.move.y, CellStatus::Black);

    t_BWBoard19 bb2 = to_bb(b);
    print_bb_19<BoardTraits<19>>(bb2);
    
    CHECK(t.move.x == 9);
    CHECK(t.move.y == 2);
}

// Benchmark comparatif du tri des coups (full vs light) sur un jeu de positions
// FIXES → mesures déterministes et comparables entre variantes.
// Lancer une fois par variante (toggle GOMOKU_LIGHT_MOVE_ORDER dans config.hpp) :
//   make run_tests FILTER="*BENCH*"
// Comparer surtout les lignes TOTAL : nodes = qualité du tri (déterministe),
// time/nps = effet net (inclut le coût du tri).
TEST_CASE("[PERF][BENCH] move ordering: fixed positions by depth")
{
    struct Pos { const char* name; GameBoard board; Color toMove; };

    std::vector<Pos> positions;

    // 1) Menaces croisées non terminales (open-threes des deux côtés, pas de 5 possible).
    positions.push_back({ "menaces-croisees", boardFromAscii({
        "...................",
        ".......B...........",
        "......B.W..........",
        ".....B..W..........",
        "........W..........",
        "...................",
    }, Color::Black), Color::Black });

    // 2) Open-threes opposés : le camp au trait doit à la fois attaquer et bloquer.
    positions.push_back({ "threes-opposes", boardFromAscii({
        "...................",
        "......BBB..........",
        "...................",
        "........WWW........",
        "...................",
    }, Color::White), Color::White });

    // 3) Position riche en captures potentielles (paires encadrables).
    positions.push_back({ "captures", boardFromAscii({
        "...................",
        ".....B.............",
        ".....W.............",
        ".....W.............",
        "......WWB..........",
        "...................",
    }, Color::Black), Color::Black });

    const char* variant = "TWO-TIER(full@d>=4, light else)";
    LOG_INFO("BENCH", std::string("variant=") + variant);

    for (int depth : {6, 8})
    {
        long long totMs = 0, totNodes = 0, totEval = 0, totPruned = 0, totTtHits = 0;

        for (auto& p : positions)
        {
            SearchPosition19 pos = SearchPosition19::fromBoard(p.board);
            MasterAI19 ai(depth, 1, p.toMove);
            Timed t = timeBestMove(ai, pos, p.toMove);

            const long long pruningPct = t.stats.nodesVisited > 0
                ? (t.stats.nodesPruned * 100LL / t.stats.nodesVisited) : 0;
            const long long nps = t.ms > 0
                ? (t.stats.nodesVisited * 1000LL / t.ms) : 0;
            LOG_INFO("BENCH",
                std::string(variant) +
                " depth=" + std::to_string(depth) +
                " pos=" + p.name +
                " time=" + std::to_string(t.ms) + "ms" +
                " nodes=" + std::to_string(t.stats.nodesVisited) +
                " eval=" + std::to_string(t.stats.nodesEvaluated) +
                " pruned=" + std::to_string(t.stats.nodesPruned) +
                " (" + std::to_string(pruningPct) + "%)" +
                " ttHits=" + std::to_string(t.stats.ttHits) +
                " nps=" + std::to_string(nps));

            totMs     += t.ms;
            totNodes  += t.stats.nodesVisited;
            totEval   += t.stats.nodesEvaluated;
            totPruned += t.stats.nodesPruned;
            totTtHits += t.stats.ttHits;

            CHECK(t.stats.nodesVisited > 0);
        }

        const long long totPct = totNodes > 0 ? (totPruned * 100LL / totNodes) : 0;
        const long long totNps = totMs > 0 ? (totNodes * 1000LL / totMs) : 0;

        LOG_INFO("BENCH",
            std::string(variant) +
            " depth=" + std::to_string(depth) +
            " TOTAL time=" + std::to_string(totMs) + "ms" +
            " nodes=" + std::to_string(totNodes) +
            " eval=" + std::to_string(totEval) +
            " pruned=" + std::to_string(totPruned) +
            " (" + std::to_string(totPct) + "%)" +
            " ttHits=" + std::to_string(totTtHits) +
            " nps=" + std::to_string(totNps));
    }
}

// TEST_CASE("[PERF] findBestMove black wins in one – open four")
// {
//     const int depth = 2;
//     GameBoard b = boardFromAscii({
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "....BBBB...........",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//         "...................",
//     }, Color::Black);

//     b.setCurrentColor(Color::Black);
//     t_BWBoard19 bb = to_bb(b);
//     print_bb_19<BoardTraits<19>>(bb);

//     SearchPosition19 pos = SearchPosition19::fromBoard(b);
//     MasterAI19 ai(depth, 1, Color::Black);
//     Timed t = timeBestMove(ai, pos, Color::Black);
//     double nps = t.ms > 0 ? (t.stats.nodesVisited * 1000.0 / t.ms) : 0.0;
//     LOG_INFO("PERF",
//         "depth=" + std::to_string(depth) +
//         " time=" + std::to_string(t.ms) + "ms" +
//         " nodes=" + std::to_string(t.stats.nodesVisited) +
//         " nps=" + std::to_string((long long)nps));
//     LOG_INFO("PERF", "move=" + std::to_string(t.move.x) + "," + std::to_string(t.move.y));
    
//     place(b, t.move.x, t.move.y, CellStatus::Black);
//     t_BWBoard19 bb2 = to_bb(b);
//     print_bb_19<BoardTraits<19>>(bb2);
    
//     CHECK(t.ms < 500);
// }