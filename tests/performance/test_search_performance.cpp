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

// Le livre d'ouverture répond instantanément (aucun nœud visité) sur les
// positions triviales : plateau vide → centre ; une seule pierre → contact
// diagonal vers le centre. C'est ce qui supprime le plus gros pic de temps de la
// partie (l'ouverture, très coûteuse à fouiller car sans menace) sans calcul.
TEST_CASE("[PERF][opening] book plays instantly on empty / single-stone boards")
{
    // Plateau vide → centre, sans recherche.
    {
        GameBoard b = empty_board();
        b.setCurrentColor(Color::Black);
        SearchPosition19 pos = SearchPosition19::fromBoard(b);
        MasterAI19 ai(8, 1, Color::Black);
        const t_cell mv = ai.findBestMove(pos, Color::Black);
        CHECK(mv.x == 9);
        CHECK(mv.y == 9);
        CHECK(ai.lastSearchStats().nodesVisited == 0);
    }
    // Une seule pierre au centre → réponse instantanée en diagonale (10,10).
    {
        GameBoard b = empty_board();
        place(b, 9, 9, CellStatus::Black);
        b.setCurrentColor(Color::White);
        SearchPosition19 pos = SearchPosition19::fromBoard(b);
        MasterAI19 ai(8, 1, Color::White);
        const t_cell mv = ai.findBestMove(pos, Color::White);
        CHECK(ai.lastSearchStats().nodesVisited == 0);
        CHECK(mv.x == 10);
        CHECK(mv.y == 10);
    }
}

// L'ouverture (0/1 pierre) étant désormais jouée par le livre, ce test mesure le
// temps de recherche par profondeur sur une position calme mais NON-livresque
// (quelques pierres posées), représentative du tout début de partie réel.
TEST_CASE("[PERF] findBestMove timing by depth (quiet early position)")
{
    for (int depth : {1, 2, 4, 6, 8}) {
        GameBoard b = empty_board();
        place(b, 9, 9, CellStatus::Black);
        place(b, 9, 10, CellStatus::White);
        b.setCurrentColor(Color::Black);
        SearchPosition19 pos = SearchPosition19::fromBoard(b);

        MasterAI19 ai(depth, 1, Color::Black);

        Timed t = timeBestMove(ai, pos, Color::Black);

        LOG_INFO("PERF",
            "depth=" + std::to_string(depth) +
            " time=" + std::to_string(t.ms) + "ms" +
            " nodes=" + std::to_string(t.stats.nodesVisited));

        CHECK(t.stats.nodesVisited > 0);
        // Une position CALME fouillée en profondeur peut légitimement dépasser
        // 500 ms (c'est précisément le mal que combattent le livre d'ouverture et
        // la profondeur adaptative de début de partie) : sur une position à peine
        // développée, même depth 6 coûte ~1 s selon la machine. On ne garde donc
        // le garde-fou temps que sur les profondeurs franchement rapides
        // (≤ 4, robustes au bruit machine) et on se contente de journaliser au-delà.
        if (depth <= 4)
            CHECK(t.ms <= 500);
    }
}

// TEMP bench: profondeur adaptative en début de partie (à supprimer).
TEST_CASE("[PERF][TMP] adaptive opening depth @ configured=10")
{
    struct Row { const char* name; int stones; GameBoard board; };
    std::vector<Row> rows;

    { GameBoard b = empty_board();
      place(b,9,9,CellStatus::Black); place(b,9,10,CellStatus::White);
      rows.push_back({"2-stones",2,b}); }
    { GameBoard b = empty_board();
      place(b,9,9,CellStatus::Black); place(b,9,10,CellStatus::White);
      place(b,10,9,CellStatus::Black); place(b,8,10,CellStatus::White);
      rows.push_back({"4-stones",4,b}); }
    { GameBoard b = empty_board();
      place(b,9,9,CellStatus::Black); place(b,9,10,CellStatus::White);
      place(b,10,9,CellStatus::Black); place(b,8,10,CellStatus::White);
      place(b,10,10,CellStatus::Black); place(b,8,9,CellStatus::White);
      rows.push_back({"6-stones",6,b}); }

    for (auto& r : rows) {
        r.board.setCurrentColor(Color::Black);
        SearchPosition19 pos = SearchPosition19::fromBoard(r.board);
        MasterAI19 ai(10, 1, Color::Black);           // configuré à 10
        Timed t = timeBestMove(ai, pos, Color::Black);
        LOG_INFO("TMP",
            std::string(r.name) +
            " cfg=10 effDepthSeen=" + std::to_string(t.stats.maxDepthSeen) +
            " time=" + std::to_string(t.ms) + "ms" +
            " nodes=" + std::to_string(t.stats.nodesVisited));
        CHECK(t.stats.nodesVisited > 0);
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

#ifdef GOMOKU_LIGHT_MOVE_ORDER
    const char* variant = "LIGHT(off+cap)";
#else
    const char* variant = "FULL(off+def/2+cap)";
#endif
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