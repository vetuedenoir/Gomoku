#ifndef WINDETECTOR_HPP
#define WINDETECTOR_HPP

#include "bitboard/bitboard.hpp"
#include "bitboard/BitboardTool.hpp"
#include "game/contracts/contracts.hpp"

#include <optional>

// fonction qui ne detecte pas les captures gagnantes, mais seulement les alignements de 5
// surement à compléter plus tard pour les captures
template<typename Traits> bool isWinAfterMove(const t_BWBoard<Traits>& bb, const Color color, int col, int row)
{
	return BitboardTool<Traits>::instance().is_five_in_a_row(bitboardForColor(bb, color), col, row);
}

// ─── Règle de la capture finale ──────────────────────────────────────────────
//
// Un cinq aligné ne gagne que si l'adversaire ne peut pas le casser en prenant
// une paire qui en contient une pierre. C'est la SEULE parade : une prise
// ailleurs sur le plateau ne compte pas, même si elle complète les cinq paires
// du défenseur — l'alignement se résout avant elle.
//
// Cas particulier : si la prise casse la ligne ET porte le défenseur à
// CAPTURES_TO_WIN, les deux victoires tombent sur le même coup et la partie est
// nulle. Le butin d'un coup n'étant pas borné à une paire, cela peut arriver
// depuis n'importe quel compteur de départ.

template<typename Traits> inline bool isEmptyCell(const t_BWBoard<Traits>& bb, int x, int y)
{
	return !get_bb_generic<Traits>(bb.black, x, y) && !get_bb_generic<Traits>(bb.white, x, y);
}

// Parcourt les prises adverses dont au moins une des deux pierres appartient à
// `stones`, en appelant `fn(landingX, landingY)` sur la case où le preneur doit
// jouer. `fn` renvoie false pour interrompre le parcours.
//
// Une paire (P,Q) de `victim` est prenable ssi l'un de ses flancs porte une
// pierre du preneur et l'autre est vide : c'est exactement le motif
// [vide][V][V][preneur] testé par detect_captures, vu depuis la victime. La case
// jouée est le flanc vide.
//
// On ne balaie donc pas le plateau : il suffit de partir de chaque pierre de
// `stones` et de regarder ses 8 demi-directions, la pierre examinée pouvant
// être le membre proche ou lointain de la paire. Le coup de prise est toujours
// légal (une capture exempte du double-trois), aucune vérification en plus.
template<typename Traits, typename Fn>
void forEachCaptureLandingWithin(const t_BWBoard<Traits>& bb, const Color victim,
                                 const typename Traits::Bitboard& stones, Fn fn)
{
	const typename Traits::Bitboard& own = bitboardForColor(bb, victim);
	const typename Traits::Bitboard& opp = bitboardForColor(bb, opponentOf(victim));

	bool stop = false;

	bb_for_each_bit<Traits>(stones,
	                        [&](int x, int y)
	                        {
								if (stop)
									return;

								for (int d = 0; d < 4 && !stop; ++d)
								{
									for (int s = 0; s < 2 && !stop; ++s)
									{
										const int stepX = (s ? 1 : -1) * dx(LINE_DIRS[d]);
										const int stepY = (s ? 1 : -1) * dy(LINE_DIRS[d]);

										// paire = (x,y) + (qx,qy) ; flancs en (bx,by) et (fx,fy).
										const int qx = x + stepX, qy = y + stepY;
										const int bx = x - stepX, by = y - stepY;
										const int fx = x + 2 * stepX, fy = y + 2 * stepY;

										if (!in_board_generic<Traits>(qx, qy) || !in_board_generic<Traits>(bx, by) ||
				                            !in_board_generic<Traits>(fx, fy))
											continue;

										if (!get_bb_generic<Traits>(own, qx, qy))
											continue; // pas une paire de la victime

										const bool backOpp   = get_bb_generic<Traits>(opp, bx, by);
										const bool backFree  = isEmptyCell<Traits>(bb, bx, by);
										const bool frontOpp  = get_bb_generic<Traits>(opp, fx, fy);
										const bool frontFree = isEmptyCell<Traits>(bb, fx, fy);

										if (backOpp && frontFree)
											stop = !fn(fx, fy);
										else if (backFree && frontOpp)
											stop = !fn(bx, by);
									}
								}
							});
}

template<typename Traits>
bool canCapturePairWithin(const t_BWBoard<Traits>& bb, const Color victim, const typename Traits::Bitboard& stones)
{
	bool capturable = false;

	forEachCaptureLandingWithin<Traits>(bb, victim, stones,
	                                    [&](int, int)
	                                    {
											capturable = true;
											return false;
										});

	return capturable;
}

// L'adversaire de `owner` peut-il casser l'alignement décrit par `fiveMask` ?
template<typename Traits>
bool canBreakFive(const t_BWBoard<Traits>& bb, const Color owner, const typename Traits::Bitboard& fiveMask)
{
	return canCapturePairWithin<Traits>(bb, owner, fiveMask);
}

template<typename Traits> bool hasAnyCapture(const t_BWBoard<Traits>& bb, const Color capturer)
{
	const Color victim = opponentOf(capturer);
	return canCapturePairWithin<Traits>(bb, victim, bitboardForColor(bb, victim));
}

template<typename Traits>
bool isFiveRefutable(const t_BWBoard<Traits>& bb, const Color owner, const typename Traits::Bitboard& fiveMask)
{
	return canBreakFive<Traits>(bb, owner, fiveMask);
}

template<typename Traits>
inline bool bitboardsIntersect(const typename Traits::Bitboard& a, const typename Traits::Bitboard& b)
{
	for (std::size_t i = 0; i < a.size(); ++i)
		if (a[i] & b[i])
			return true;
	return false;
}

template<typename Traits>
bool hasDrawingBreak(const t_BWBoard<Traits>& bb, const Color owner, const typename Traits::Bitboard* masks, int n,
                     int defenderCaptures)
{
	const Color defender = opponentOf(owner);
	bool        found    = false;

	forEachCaptureLandingWithin<Traits>(bb, owner, masks[0],
	                                    [&](int lx, int ly)
	                                    {
											typename Traits::Bitboard captured{};
											detect_captures<Traits>(bb, lx, ly, defender, captured);

											if (defenderCaptures + popcount_bb_generic<Traits>(captured) <
		                                        CAPTURES_TO_WIN)
												return true;

											for (int i = 0; i < n; ++i)
											{
												if (!bitboardsIntersect<Traits>(captured, masks[i]))
													return true;
											}

											found = true;
											return false;
										});

	return found;
}

// Verdict d'un coup vis-à-vis de l'alignement :
//   None    — pas de cinq
//   Won     — cinq imparable, la partie s'arrête
//   Draw    — cinq cassable par une prise qui porte le défenseur à dix : nulle
//   Pending — cinq réfutable : l'adversaire a un coup pour le casser
enum class FiveVerdict
{
	None,
	Won,
	Draw,
	Pending
};

template<typename Traits>
FiveVerdict judgeFiveAfterMove(const t_BWBoard<Traits>& bb, const Color mover, int col, int row, int defenderCaptures)
{
	using Tool = BitboardTool<Traits>;

	typename Traits::Bitboard masks[Tool::MAX_FIVE_MASKS];
	const int n = Tool::instance().find_five_masks(bitboardForColor(bb, mover), col, row, masks, Tool::MAX_FIVE_MASKS);
	if (n == 0)
		return FiveVerdict::None;

	for (int i = 0; i < n; ++i)
	{
		if (!isFiveRefutable<Traits>(bb, mover, masks[i]))
			return FiveVerdict::Won;
	}

	if (hasDrawingBreak<Traits>(bb, mover, masks, n, defenderCaptures))
		return FiveVerdict::Draw;

	return FiveVerdict::Pending;
}

template<typename Traits> bool pendingFiveSurvives(const t_BWBoard<Traits>& bb, const PendingWin& pending)
{
	return isWinAfterMove<Traits>(bb, pending.owner, pending.col, pending.row);
}

template<typename Traits> std::optional<PendingWin> findExistingFive(const t_BWBoard<Traits>& bb, const Color owner)
{
	const typename Traits::Bitboard& stones = bitboardForColor(bb, owner);
	std::optional<PendingWin>        found;

	bb_for_each_bit<Traits>(stones,
	                        [&](int x, int y)
	                        {
								if (found.has_value())
									return;
								if (isWinAfterMove<Traits>(bb, owner, x, y))
									found = PendingWin{ owner, x, y };
							});

	return found;
}

#endif
