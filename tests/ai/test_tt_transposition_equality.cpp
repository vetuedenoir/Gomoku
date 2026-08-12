#include "doctest.h"
#include "ai/SearchPosition.hpp"
#include "optimization/TranspositionTable.hpp"
#include "game/contracts/contracts.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Propriété de TRANSPOSITION.
//
// Deux ordres de coups différents menant à la MÊME position (mêmes pierres, même
// trait) doivent produire le MÊME hash Zobrist. C'est la propriété sur laquelle
// repose le taux de hits de la table de transposition — et donc tout le bénéfice
// de l'iterative deepening + de l'ordonnancement par bestMove.
//
// Régression visée : si le hash incrémental dépendait de l'ORDRE des coups (et
// pas seulement de la position finale), la TT ne retrouverait presque jamais une
// position atteinte par un autre chemin → ordering/cutoffs inefficaces, ID sans
// gain. Ici on le vérifie directement.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Joue un coup NON capturant : met à jour occupation + hash comme en recherche.
void playSimple(SearchPosition19& pos, t_cell cell, Color color)
{
    const MoveStateHash msh = pos.buildMoveHash(EvaluatedMove{ cell, 0, true, {} }, color);
    pos.makeMove(cell.x, cell.y, color, msh);
}

} // namespace

TEST_CASE("[Zobrist][TT] transposition: different move orders -> same hash")
{
    GameBoard b = empty_board();
    b.setCurrentColor(Color::Black);

    // 4 cases éloignées : aucune capture, aucune adjacence possible.
    const t_cell B1{ 3, 3 }, B2{ 15, 15 };  // coups noirs
    const t_cell W1{ 3, 15 }, W2{ 15, 3 };  // coups blancs

    // Ordre 1 : B1, W1, B2, W2
    SearchPosition19 p1 = SearchPosition19::fromBoard(b);
    playSimple(p1, B1, Color::Black);
    playSimple(p1, W1, Color::White);
    playSimple(p1, B2, Color::Black);
    playSimple(p1, W2, Color::White);

    // Ordre 2 : B2, W2, B1, W1  → même position finale.
    SearchPosition19 p2 = SearchPosition19::fromBoard(b);
    playSimple(p2, B2, Color::Black);
    playSimple(p2, W2, Color::White);
    playSimple(p2, B1, Color::Black);
    playSimple(p2, W1, Color::White);

    // Propriété fondamentale : hash identique et même trait.
    CHECK(p1.zobristHash() == p2.zobristHash());
    CHECK(p1.sideToMove() == p2.sideToMove());

    // Conséquence concrète pour la TT : une entrée stockée via un ordre est
    // retrouvée via l'autre (ce dont l'ID/ordering a besoin).
    TranspositionTable tt;
    tt.store(p1.zobristHash(), 4242, 6, TTFlag::Exact, B1);

    const TTEntry* hit = tt.probe(p2.zobristHash());
    REQUIRE(hit != nullptr);
    CHECK(hit->score == 4242);
    CHECK(hit->bestMove.x == B1.x);
    CHECK(hit->bestMove.y == B1.y);
}

// Contrôle négatif : deux positions RÉELLEMENT différentes (une pierre en moins)
// ne doivent pas hasher pareil — garde-fou anti « hash constant ».
TEST_CASE("[Zobrist][TT] transposition: distinct positions -> distinct hash")
{
    GameBoard b = empty_board();
    b.setCurrentColor(Color::Black);

    SearchPosition19 full = SearchPosition19::fromBoard(b);
    playSimple(full, t_cell{ 3, 3 }, Color::Black);
    playSimple(full, t_cell{ 3, 15 }, Color::White);

    SearchPosition19 partial = SearchPosition19::fromBoard(b);
    playSimple(partial, t_cell{ 3, 3 }, Color::Black);

    CHECK(full.zobristHash() != partial.zobristHash());
}
