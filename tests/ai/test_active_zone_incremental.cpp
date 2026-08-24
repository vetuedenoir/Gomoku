#include "doctest.h"
#include "ai/SearchPosition.hpp"
#include "ai/ActiveZone.hpp"
#include "game/contracts/contracts.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Zone active incrémentale (SearchPosition::candidateMask)
//
// Propriété testée : le masque de candidats maintenu de façon incrémentale par
// SearchPosition (ref-counting mis à jour dans makeMove/undoMove) doit être
// STRICTEMENT identique au masque recalculé « à froid » par ActiveZone sur
// l'occupation réelle du plateau — y compris après des captures, et après un
// aller-retour make/undo.
//
// ActiveZone (le calcul plein scan que l'on remplace) sert de vérité terrain.
// Le rayon utilisé par SearchPosition (ZONE_RADIUS = 1) correspond au radius
// employé par MasterAI, donc on instancie ActiveZone(1) pour comparer.
// ────────────────────────────────────────────────────────────────────────────

using T19 = BoardTraits<19>;

static bool masksEqual(const T19::Bitboard& a, const T19::Bitboard& b)
{
	for (int i = 0; i < T19::WORD_COUNT; ++i)
		if (a[i] != b[i])
			return false;
	return true;
}

// Vérité terrain : la zone active recalculée à froid (radius 1) sur `board`.
static T19::Bitboard coldZone(const t_BWBoard19& board)
{
	ActiveZone<T19> zone(1);
	zone.initialize(board);
	return zone.getCandidateMask();
}

// Un plateau de mi-partie avec pierres des deux couleurs et des captures
// potentielles autour.
static GameBoard midgameBoard()
{
	GameBoard b = empty_board();
	place(b, 9, 9, CellStatus::Black);
	place(b, 10, 9, CellStatus::White);
	place(b, 8, 10, CellStatus::Black);
	place(b, 11, 8, CellStatus::White);
	place(b, 7, 7, CellStatus::Black);
	place(b, 12, 12, CellStatus::White);
	b.setCurrentColor(Color::Black);
	return b;
}

TEST_CASE("[Zone] candidateMask == ActiveScan à froid (mi-partie)")
{
	GameBoard        b   = midgameBoard();
	SearchPosition19 pos = SearchPosition19::fromBoard(b);

	CHECK(masksEqual(pos.candidateMask(), coldZone(pos.board())));
}

TEST_CASE("[Zone] plateau vide : amorce le centre comme ActiveZone")
{
	GameBoard        b   = empty_board();
	SearchPosition19 pos = SearchPosition19::fromBoard(b);

	const T19::Bitboard inc = pos.candidateMask();
	CHECK(masksEqual(inc, coldZone(pos.board())));
	CHECK(get_bb_generic<T19>(inc, 19 / 2, 19 / 2));
}

TEST_CASE("[Zone] make/undo SANS capture restaure la zone à l'identique")
{
	GameBoard        b   = midgameBoard();
	SearchPosition19 pos = SearchPosition19::fromBoard(b);

	const T19::Bitboard before = pos.candidateMask();

	// Un coup vide, hors de tout motif de capture -> caps = 0.
	EvaluatedMove move{};
	move.move = { 5, 5 };

	const MoveStateHash msh = pos.buildMoveHash(move, Color::Black);
	pos.makeMove(move.move.x, move.move.y, Color::Black, msh);

	// Après le coup, l'incrémental doit coïncider avec un recalcul à froid.
	CHECK(masksEqual(pos.candidateMask(), coldZone(pos.board())));

	pos.undoMove(move.move.x, move.move.y, Color::Black);

	// Et l'aller-retour doit restaurer exactement l'état initial.
	CHECK(masksEqual(pos.candidateMask(), before));
}

TEST_CASE("[Zone] make/undo AVEC capture restaure la zone à l'identique")
{
	// Capture horizontale : B[posé en 5,9] W(6,9) W(7,9) B(8,9).
	GameBoard b = empty_board();
	place(b, 6, 9, CellStatus::White);
	place(b, 7, 9, CellStatus::White);
	place(b, 8, 9, CellStatus::Black);
	b.setCurrentColor(Color::Black);

	SearchPosition19    pos    = SearchPosition19::fromBoard(b);
	const T19::Bitboard before = pos.candidateMask();

	EvaluatedMove move{};
	move.move        = { 5, 9 };
	move.captureMask = detect_capture_mask(pos.board(), 5, 9, Color::Black);
	REQUIRE(capture_mask_count(move.captureMask) == 2);

	const MoveStateHash msh = pos.buildMoveHash(move, Color::Black);
	pos.makeMove(move.move.x, move.move.y, Color::Black, msh);

	// Les deux blancs ont disparu : la zone incrémentale doit refléter le retrait
	// de leur support, exactement comme un recalcul plein scan.
	CHECK(masksEqual(pos.candidateMask(), coldZone(pos.board())));

	// Les cases libérées, désormais voisines de pierres noires, redeviennent
	// candidates : preuve que le -1 sur les victimes n'a pas « effacé » leur zone.
	CHECK(get_bb_generic<T19>(pos.candidateMask(), 6, 9));
	CHECK(get_bb_generic<T19>(pos.candidateMask(), 7, 9));

	pos.undoMove(move.move.x, move.move.y, Color::Black);

	// Restauration des victimes -> zone identique à l'état initial.
	CHECK(masksEqual(pos.candidateMask(), before));
}
