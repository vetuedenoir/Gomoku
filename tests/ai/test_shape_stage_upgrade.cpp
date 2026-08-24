#include "doctest.h"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/board.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/master_ai_test_access.hpp"
#include <vector>

// ────────────────────────────────────────────────────────────────────────────
// Ordonnancement : computeLightScore + upgradeLightToFull ≡ rawShapeScoreV2.
//
// Le tri de minimax ne recalcule plus la clé full depuis zéro : il repart de la
// clé light et n'exécute que le maillon que celle-ci saute (check_cross), en se
// fiant au `stage` pour savoir si ce maillon peut encore changer le verdict.
// Ce test est le filet de sécurité de cette hypothèse : sur toutes les cases
// vides de plusieurs positions, pour les deux couleurs et plusieurs compteurs de
// capture, les deux chemins doivent produire le même score, le même masque de
// captures et le même étage.
//
// La légalité suit la même règle que StandardRules : un four exempte le
// double-trois, une croix non. Light et V2 doivent donc s'accorder.
// ────────────────────────────────────────────────────────────────────────────

using Access = MasterAITestAccess<BoardTraits<19>>;

namespace {

struct NamedBoard { const char* name; GameBoard board; };

std::vector<NamedBoard> orderingPositions()
{
    std::vector<NamedBoard> positions;

    positions.push_back({ "menaces-croisees", boardFromAscii({
        "...................",
        ".......B...........",
        "......B.W..........",
        ".....B..W..........",
        "........W..........",
    }, Color::Black) });

    positions.push_back({ "threes-opposes", boardFromAscii({
        "...................",
        "......BBB..........",
        "...................",
        "........WWW........",
    }, Color::White) });

    positions.push_back({ "captures", boardFromAscii({
        "...................",
        ".....B.............",
        ".....W.............",
        ".....W.............",
        "......WWB..........",
    }, Color::Black) });

    // Croix et double-trois enchevêtrés : V2 sort au check_cross, light au
    // open_three ; la légalité du double-trois doit rester la même.
    positions.push_back({ "croix", boardFromAscii({
        "...................",
        "....B..............",
        ".....B.............",
        "..BB.............BB",
        ".....B.............",
        "....B..............",
    }, Color::Black) });

    return positions;
}

} // namespace

TEST_CASE("[Ordering] light+upgrade reproduit exactement la clé full")
{
    MasterAI19 ai(4, 1, Color::Black);

    int upgraded = 0;

    for (NamedBoard& position : orderingPositions())
    {
        const t_BWBoard19 board = SearchPosition19::fromBoard(position.board).board();

        for (int captures : { 0, 4, 8 })
        {
            for (Color color : { Color::Black, Color::White })
            {
                for (int y = 0; y < 19; ++y)
                {
                    for (int x = 0; x < 19; ++x)
                    {
                        if (get_bb_generic<BoardTraits<19>>(board.black, x, y) ||
                            get_bb_generic<BoardTraits<19>>(board.white, x, y))
                            continue;

                        const t_cell cell { static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y) };

                        EvaluatedMove light = Access::lightKey(ai, board, cell, color, captures);
                        const ShapeStage lightStage = light.stage;
                        Access::upgrade(ai, light, board, color, captures);

                        const EvaluatedMove full = Access::fullKey(ai, board, cell, color, captures);

                        INFO("pos=" << position.name << " color=" << (color == Color::Black ? "B" : "W")
                             << " captures=" << captures << " cell=(" << x << "," << y << ")");

                        CHECK(light.score == full.score);
                        CHECK(light.stage == full.stage);
                        CHECK(light.captureMask == full.captureMask);
                        CHECK(light.isLegal == full.isLegal);

                        if (lightStage == ShapeStage::ThreeOrQuiet && light.stage == ShapeStage::Cross)
                            ++upgraded;
                    }
                }
            }
        }
    }

    // Garde-fou : si plus aucune case ne remonte de croix, le test ne prouve
    // plus rien du maillon qu'il est censé couvrir.
    CHECK(upgraded > 0);
}
