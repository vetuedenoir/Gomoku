#include "doctest.h"
#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/board.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"

// ────────────────────────────────────────────────────────────────────────────
// Coups forcés : quand l'adversaire aligne cinq au coup suivant, la liste des
// candidats est réduite aux réponses qui peuvent encore changer l'issue.
//
// Les deux sens comptent. Ne pas se déclencher coûte du temps ; se déclencher à
// tort supprime des coups jouables et fausse la recherche — d'où le contrôle sur
// une position calme.
// ────────────────────────────────────────────────────────────────────────────

TEST_CASE("[Forced] un quatre adverse réduit la recherche aux parades")
{
	// Blanc tient WWWW en (5..8, 5) : Noir doit occuper (4,5) ou (9,5). Noir n'a
	// aucune pierre, donc aucune capture ne peut lui servir de parade.
	GameBoard b = boardFromAscii(
		{
			"...................",
			"...................",
			"...................",
			"...................",
			"...................",
			".....WWWW..........",
		},
		Color::Black);

	SearchPosition19 pos = SearchPosition19::fromBoard(b);
	MasterAI19       ai(4, 1, Color::Black);

	const t_cell move = ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().forcedNodes > 0);
	const bool blocks = (move.x == 4 && move.y == 5) || (move.x == 9 && move.y == 5);
	CHECK(blocks);
}

TEST_CASE("[Forced] une position calme n'est pas restreinte")
{
	// Le compteur porte sur tout l'arbre, et une recherche profonde atteint
	// forcément des positions où un quatre existe. On cherche donc à
	// profondeur 1 pour n'observer que la racine, elle bien calme.
	GameBoard b = boardFromAscii(
		{
			"...................",
			".......B...........",
			"......B.W..........",
			".....B..W..........",
		},
		Color::Black);

	SearchPosition19 pos = SearchPosition19::fromBoard(b);
	MasterAI19       ai(1, 1, Color::Black);

	ai.findBestMove(pos, Color::Black);

	CHECK(ai.lastSearchStats().forcedNodes == 0);
}
