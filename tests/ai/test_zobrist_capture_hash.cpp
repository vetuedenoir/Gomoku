#include "doctest.h"
#include "ai/SearchPosition.hpp"
#include "game/contracts/contracts.hpp"
#include "optimization/ZobristHasher.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"

// ────────────────────────────────────────────────────────────────────────────
// buildMoveHash : cohérence du hash Zobrist sur une capture.
//
// Propriété testée : le hash incrémental produit par buildMoveHash pour un coup
// (y compris les captures) doit être IDENTIQUE au hash recalculé « à froid » sur
// l'occupation réelle du plateau après le coup.
//
// compute() (ZobristHasher) ne hache QUE l'occupation : ni le trait, ni les
// compteurs de capture. buildMoveHash ajoute en plus sideKey() (bascule du
// trait) et captureHash() (compteur de paires). On neutralise donc ces deux
// composantes dans l'attendu pour comparer proprement.
//
// Régression visée : si buildMoveHash oublie de retirer (XOR) les clés des
// pierres capturées, le hash conserve des pierres fantômes → la TT renvoie de
// faux résultats. Ici le delta serait exactement les clés des victimes.
// ────────────────────────────────────────────────────────────────────────────

// Capture horizontale  B[posé] W W B  →  Noir pose en (5,9), capture (6,9)(7,9).
TEST_CASE("[Zobrist] buildMoveHash retire les clés des pierres capturées")
{
    GameBoard before = empty_board();
    place(before, 6, 9, CellStatus::White);
    place(before, 7, 9, CellStatus::White);
    place(before, 8, 9, CellStatus::Black);
    before.setCurrentColor(Color::Black);

    SearchPosition19 pos = SearchPosition19::fromBoard(before);

    // Le coup tel que rawShapeScore le remplirait : la case posée + le masque
    // des captures, d'où buildMoveHash redéduit les victimes (6,9) et (7,9).
    EvaluatedMove move {};
    move.move = { 5, 9 };
    move.captureMask = detect_capture_mask(pos.board(), 5, 9, Color::Black);
    REQUIRE(capture_mask_count(move.captureMask) == 2);

    const MoveStateHash msh = pos.buildMoveHash(move, Color::Black);

    // Vérité terrain indépendante : l'occupation APRÈS le coup, recalculée à froid.
    // Les deux blancs ont disparu, un noir apparaît en (5,9), le trait passe à Blanc.
    GameBoard after = empty_board();
    place(after, 8, 9, CellStatus::Black);
    place(after, 5, 9, CellStatus::Black);
    after.setCurrentColor(Color::White);

    SearchPosition19 posAfter = SearchPosition19::fromBoard(after);

    const ZobristHasher<BoardTraits<19>>& H = SearchPosition19::hasher();

    // buildMoveHash ajoute le trait + la clé de compteur (1 paire prise par Noir),
    // absents d'un compute() d'occupation pure.
    const uint64_t expected = posAfter.zobristHash()
                            ^ H.sideKey()
                            ^ H.captureHash(Color::White, 1);

    CHECK(msh.hash == expected);

    // Démonstration ciblée du bug : l'ancienne version (sans le retrait des
    // victimes) produirait ce hash-là, qui diffère de l'attendu d'EXACTEMENT les
    // deux clés des pierres capturées.
    const uint64_t withoutRemoval = msh.hash
                                  ^ H.key(6, 9, Color::White)
                                  ^ H.key(7, 9, Color::White);
    CHECK(withoutRemoval != expected);
    CHECK(withoutRemoval == (expected ^ H.key(6, 9, Color::White)
                                      ^ H.key(7, 9, Color::White)));
}

// Contrôle : un coup SANS capture doit aussi être cohérent (occupation + trait,
// pas de clé de compteur). Garde le chemin nominal de buildMoveHash.
TEST_CASE("[Zobrist] buildMoveHash cohérent pour un coup sans capture")
{
    GameBoard before = empty_board();
    place(before, 8, 9, CellStatus::Black);
    before.setCurrentColor(Color::Black);

    SearchPosition19 pos = SearchPosition19::fromBoard(before);

    EvaluatedMove move {};
    move.move = { 2, 2 };

    const MoveStateHash msh = pos.buildMoveHash(move, Color::Black);

    GameBoard after = empty_board();
    place(after, 8, 9, CellStatus::Black);
    place(after, 2, 2, CellStatus::Black);
    after.setCurrentColor(Color::White);

    SearchPosition19 posAfter = SearchPosition19::fromBoard(after);

    const ZobristHasher<BoardTraits<19>>& H = SearchPosition19::hasher();

    const uint64_t expected = posAfter.zobristHash() ^ H.sideKey();

    CHECK(msh.hash == expected);
}
