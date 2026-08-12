#include "ai/MasterAI.hpp"
#include "game/turn/WinDetector.hpp"
#include "logger/Logger.hpp"
// les coups impliquant des captures doivent toujours etre mieux evalues que les coups sans capture.

// 1 MILLIONS = victoir
// superieur a 100 000 = coups imparables
// superieur a 10 000 = coups imparable dans les 2 prochains tours,
//						comme un double three ou une croix

// superieur a 1000 = coups avec 1 chance de parade, broken four

// superieur a 100 = coups avec 2 chances de parade,
//						comme une croix avec un seul coté ouvert, ou un three ouvert

// inferieur a 100 = coups avec 3 chances de parade ou plus, comme une croix avec les 2 cotés fermés, ou un three demi-ouvert

static int cross_score(int crossResult)
{
    switch (crossResult)
    {
        case CROSS_FULL:
        case CROSS_FULL_OPP_EXTERN:
            return 90000;

        case CROSS_DEMI_NO_MID:
            return 2000;

        case CROSS_DEMI_MID:
            return 1500;

        case CROSS_FULL_OPP_INTERN:
            return 9000;

        case CROSS_DEMI_NO_OPP_EXTERN:
            return 300;

        case CROSS_DEMI_MID_OPP_EXTERN:
            return 350;

        case CROSS_DEMI_NO_OPP_INTERN:
            return 150;

        case CROSS_DEMI_MID_OPP_INTERN:
            return 175;

        default:
            return 0;
    }
}

// Reflechir a enleve les verifications doublons CROSS_FULL et CROSS_FULL_OPP_EXTERN INTERN
// qui sont pareil que les double three SCORE_DOUBLE_FULL_FULL

static int score_open_three(int threeResult)
{
	// LOG_DEBUG("AI", "[score_open_three] threeResult=" + std::to_string(threeResult));
    
	switch (threeResult)
    {
        case SCORE_3_FULL:  return 800;
        case SCORE_3_HOLE:  return 700;

        case SCORE_FULL_EXTERN: return 500;
        case SCORE_HOLE_EXTERN: return 450;

        case SCORE_FULL_INTERN: return 40;
		// peut etre meme en dessous de 100, car peut permettre une capture
        case SCORE_HOLE_INTERN: return 35;

        case SCORE_DOUBLE_FULL_FULL:
        case SCORE_DOUBLE_FULL_FULL_EXTERN:
            return 90000;

        case SCORE_DOUBLE_HOLE_FULL:
        case SCORE_DOUBLE_HOLE_FULL_EXTERN:
            return 85000;

        case SCORE_DOUBLE_HOLE_HOLE:
        case SCORE_DOUBLE_HOLE_HOLE_EXTERN:
            return 80000;

		// Necessite une defense en 3 coups pour etre pare, pas d'erreur possible
        case SCORE_DOUBLE_FULL_FULL_INTERN:
            return 9000;
        case SCORE_DOUBLE_HOLE_FULL_INTERN:
            return 8500;
        case SCORE_DOUBLE_HOLE_HOLE_INTERN:
            return 8000;

		// Necessite une defense en 2 coups pour etre pare, pas d'erreur possible
        case SCORE_DOUBLE_FULL_FULL_MIXED:
            return 7500;
        case SCORE_DOUBLE_HOLE_FULL_MIXED:
            return 7000;
        case SCORE_DOUBLE_HOLE_HOLE_MIXED:
            return 6500;

        case SCORE_DOUBLE_FULL_FULL_INTERN2:
            return 90;
        case SCORE_DOUBLE_HOLE_FULL_INTERN2:
            return 85;
        case SCORE_DOUBLE_HOLE_HOLE_INTERN2:
            return 80;

        default:
            return 0;
    }
}

// Score "brut" (non signé par rapport à l'IA) de la forme créée en posant
// `color` en `cell`. Les barèmes sont ceux de evaluatePosition, mais ici la
// pierre est posée de façon hypothétique dans une copie du plan de couleur
// (les masques des check_* incluent la case et exigent la pierre présente).


template <typename Traits>
EvaluatedMove MasterAI<Traits>::rawShapeScore(const t_BWBoard<Traits>& board, t_cell cell, Color color, int capturesBefore)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y); // pose hypothétique (copie locale)

	EvaluatedMove data {};
	data.isLegal = true;
	data.move = cell;
	data.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps = capture_mask_count(data.captureMask);

	const int captureScore = captureProgressScore(capturesBefore, caps);

	if (capturesBefore + caps >= 10)
	{
		data.score = 1000000 + captureScore;
		return data;
	}

	data.score = captureScore + 89;
	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		data.score = 1000000 + captureScore;
		data.stage = ShapeStage::Terminal;
		return data;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	if (r == 2)
	{
		data.score = 500000 + captureScore;
		data.stage = ShapeStage::OpenFour;
		return data;
	}
	if (r == 1) {
		data.score = 5000 + captureScore;
		data.stage = ShapeStage::HalfFour;
		return data;
	}

	// if (tool.check_super_four(own, opp, cell.x, cell.y))
	// {
	// 	data.score = 60000 + captureScore;
	// 	return data;
	// }
	if (tool.check_broken_four(own, opp, cell.x, cell.y))
	{
		data.score = 6000 + captureScore;
		data.stage = ShapeStage::BrokenFour;
		return data;
	}

	r = tool.check_cross(own, opp, cell.x, cell.y);
	if (r) 
	{
		data.score = cross_score(r) + captureScore;
		data.stage = ShapeStage::Cross;
		return data;
	}

	r = tool.check_open_three(own, opp, cell.x, cell.y);
	if (r)
	{
		data.score = score_open_three(r) + captureScore;
		// Exemption = capture ON THIS MOVE only (not race progress).
		data.isLegal = !(tool.isDoubleThreeScore(r) && caps == 0);
		return data;
	}
	return data;
}


// Clé « full » de référence. Le tri de minimax ne l'appelle plus : il part de
// rawShapeScoreLight et complète avec upgradeLightToFull, qui produit le même
// résultat pour un seul scan de motif au lieu de cinq. L'équivalence des deux
// chemins est verrouillée par [Ordering] light+upgrade ≡ V2.
template <typename Traits>
EvaluatedMove MasterAI<Traits>::rawShapeScoreV2(const t_BWBoard<Traits>& board, t_cell cell, Color color, int captureCount)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y); // pose hypothétique (copie locale)

	EvaluatedMove data {};
	data.isLegal = true;
	data.move = cell;
	data.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps = capture_mask_count(data.captureMask);

	const int captureScore = captureProgressScore(captureCount, caps);

	if ((captureCount + caps) >= 10)
	{
		data.score = 1000000 + captureScore;
		data.stage = ShapeStage::Terminal;
		return data;
	}

	data.score = captureScore + 89;
	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		data.score = 1000000 + captureScore;
		data.stage = ShapeStage::Terminal;
		return data;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	if (r == 2)
	{
		data.score = 500000 + captureScore;
		data.stage = ShapeStage::OpenFour;
		return data;
	}
	if (r == 1) {
		data.score = 5000 + captureScore;
		data.stage = ShapeStage::HalfFour;
		return data;
	}

	// if (tool.check_super_four(own, opp, cell.x, cell.y))
	// {
	// 	data.score = 60000 + captureScore;
	// 	return data;
	// }
	if (tool.check_broken_four(own, opp, cell.x, cell.y))
	{
		data.score = 6000 + captureScore;
		data.stage = ShapeStage::BrokenFour;
		return data;
	}

	r = tool.check_cross(own, opp, cell.x, cell.y);
	if (r) 
	{
		data.score = cross_score(r) + captureScore;
		data.stage = ShapeStage::Cross;
		return data;
	}

	r = tool.check_open_three(own, opp, cell.x, cell.y);
	if (r)
	{
		data.score = score_open_three(r) + captureScore;
		// Exemption = capture ON THIS MOVE only (not race progress).
		data.isLegal = !(tool.isDoubleThreeScore(r) && caps == 0);
		return data;
	}
	return data;
}

// Clé light pour nœuds profonds : même captures/légalité que V2, mais sans
// check_cross (coûteux et redondant avec open_three pour le tri).
// `stage` mémorise l'étage de sortie : c'est ce qui permet à upgradeLightToFull
// de savoir si le check_cross omis peut encore changer quelque chose.
template <typename Traits>
EvaluatedMove MasterAI<Traits>::rawShapeScoreLight(const t_BWBoard<Traits>& board, t_cell cell, Color color, int captureCount)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove data {};
	data.isLegal = true;
	data.move = cell;
	data.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps = capture_mask_count(data.captureMask);
	const int captureScore = captureProgressScore(captureCount, caps);

	if ((captureCount + caps) >= 10)
	{
		data.score = 1000000 + captureScore;
		data.stage = ShapeStage::Terminal;
		return data;
	}

	data.score = captureScore + 89;
	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		data.score = 1000000 + captureScore;
		data.stage = ShapeStage::Terminal;
		return data;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	if (r == 2)
	{
		data.score = 500000 + captureScore;
		data.stage = ShapeStage::OpenFour;
		return data;
	}
	if (r == 1)
	{
		data.score = 5000 + captureScore;
		data.stage = ShapeStage::HalfFour;
		return data;
	}

	if (tool.check_broken_four(own, opp, cell.x, cell.y))
	{
		data.score = 6000 + captureScore;
		data.stage = ShapeStage::BrokenFour;
		return data;
	}

	r = tool.check_open_three(own, opp, cell.x, cell.y);
	if (r)
	{
		data.score = score_open_three(r) + captureScore;
		data.isLegal = !(tool.isDoubleThreeScore(r) && !caps);
		return data;
	}
	return data;
}

// Promeut une clé light en clé full. Light et V2 sont la même chaîne de tests à
// un maillon près : le check_cross intercalé entre le broken-four et le three.
// Tout ce que light a conclu au-dessus de cet étage (terminal, four, broken
// four) est donc déjà le verdict de V2 — captures comprises, ce qui évite de
// relancer detect_capture_mask. Il ne reste à traiter que les coups sortis
// à l'étage three/quiet, et un seul scan suffit.
// La légalité n'est jamais réévaluée : celle de light (`!caps`) est la bonne, et
// V2 la relâchait à tort dès que le camp avait déjà capturé une paire.
template <typename Traits>
void MasterAI<Traits>::upgradeLightToFull(EvaluatedMove& move, const t_BWBoard<Traits>& board,
	Color color, int captureCount)
{
	if (move.stage != ShapeStage::ThreeOrQuiet)
		return;

	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, move.move.x, move.move.y);

	const int r = tool.check_cross(own, opp, move.move.x, move.move.y);
	if (!r)
		return;

	const int caps = capture_mask_count(move.captureMask);
	move.score = cross_score(r) + captureProgressScore(captureCount, caps);
	move.stage = ShapeStage::Cross;
}

// Un cinq aligné gagne inconditionnellement (cf. isWinAfterMove) : si l'adversaire
// en pose un au coup suivant, seules trois familles de réponses peuvent encore
// changer l'issue — gagner immédiatement, occuper la case du cinq, ou capturer
// (une prise casse l'alignement, et la dixième pierre gagne). Tout le reste perd,
// et les explorer ne fait que gonfler le facteur de branchement là où l'arbre est
// le plus profond.
template <typename Traits>
bool MasterAI<Traits>::restrictToForcedReplies(const t_BWBoard<Traits>& board,
	MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits>>& ordered, Color mover)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	const Color oppColor = (mover == Color::Black) ? Color::White : Color::Black;
	const typename Traits::Bitboard& oppStones = bitboardForColor(board, oppColor);

	bool isBlock[MAX_BOARD_MOVES<Traits>] = {};
	bool threatened = false;

	// Une seule copie du plan adverse, la pierre hypothétique est posée puis
	// retirée à chaque essai.
	typename Traits::Bitboard opp = oppStones;
	for (size_t i = 0; i < ordered.size(); ++i)
	{
		const t_cell& m = ordered[i].move;
		set_bb_generic<Traits>(opp, m.x, m.y);
		if (tool.is_five_in_a_row(opp, m.x, m.y))
		{
			isBlock[i] = true;
			threatened = true;
		}
		clear_bit_generic<Traits>(opp, m.x, m.y);
	}

	if (!threatened)
		return false;

	size_t kept = 0;
	for (size_t i = 0; i < ordered.size(); ++i)
	{
		if (isBlock[i] || ordered[i].captureMask || ordered[i].score >= WIN_SCORE)
			ordered[kept++] = ordered[i];
	}

	// La case du cinq peut être interdite au camp au trait (double-trois) et
	// n'apparaître dans aucune des trois familles : mieux vaut alors chercher
	// normalement que rendre le nœud stérile.
	if (kept == 0)
		return false;

	ordered.count = kept;
	return true;
}

// Clé de tri : offense (déjà dans `offense.score`, y compris captures du camp
// au trait) + défense/2 (ce que l'adversaire créerait sur cette case s'il y
// jouait). On ne touche ni isLegal ni captureMask — réservés au makeMove
// du camp au trait.
template <typename Traits>
void MasterAI<Traits>::addDefenseToOrderingScore(EvaluatedMove& offense,
	const t_BWBoard<Traits>& board, Color side, int oppCapturesBefore)
{
	const Color opp = (side == Color::Black) ? Color::White : Color::Black;
	const EvaluatedMove defense = rawShapeScore(board, offense.move, opp, oppCapturesBefore);
	offense.score += defense.score / 2;
}

// Meilleure menace offensive près de `anchor` (réponse locale au dernier coup).
// ∩ candidateMask, early-exit dès THREAT_SCAN_EARLY_EXIT (90k).
template <typename Traits>
int MasterAI<Traits>::bestThreatNear(const SearchPosition<Traits>& position, Color color, t_cell anchor)
{
	const t_BWBoard<Traits>& board = position.board();
	const auto zone = position.candidateMask();
	const int capsBefore = position.getCapturesForColor(color);
	int best = 0;

	const int x0 = static_cast<int>(anchor.x);
	const int y0 = static_cast<int>(anchor.y);

	for (int dy = -LEAF_SCAN_RADIUS; dy <= LEAF_SCAN_RADIUS; ++dy)
	{
		for (int dx = -LEAF_SCAN_RADIUS; dx <= LEAF_SCAN_RADIUS; ++dx)
		{
			const int x = x0 + dx;
			const int y = y0 + dy;
			if (!in_board_generic<Traits>(x, y))
				continue;
			if (!get_bb_generic<Traits>(zone, x, y))
				continue;

			const t_cell cand{static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y)};
			const EvaluatedMove offense = rawShapeScore(board, cand, color, capsBefore);
			if (!offense.isLegal)
				continue;
			if (offense.score > best)
			{
				best = offense.score;
				if (best >= THREAT_SCAN_EARLY_EXIT)
					return best;
			}
		}
	}
	return best;
}

// Feuille bilatérale (coût adaptatif) :
//   createdPatterns  = formes/captures du coup qui vient d'être joué (signé pour l'IA)
//   si |createdPatterns| < THREAT_SCAN_THRESHOLD → Pas de scan
//   sinon sideToMoveBest = meilleure ofense SideToMove près de cell
//   score    = created + signedFromAi(stm, stmBest)
template <typename Traits>
int MasterAI<Traits>::evaluateLeafPosition(const SearchPosition<Traits>& position, t_cell cell)
{
	const Color sideToMove = position.sideToMove();
	const Color lastPlayed = (sideToMove == Color::Black) ? Color::White : Color::Black;

	// TODO: 
	const int createdPatterns = (lastPlayed == Color::Black)
		? evaluateBlackPosition(position, cell)
		: evaluateWhitePosition(position, cell);

	if (createdPatterns >= MATE_THRESHOLD || createdPatterns <= -MATE_THRESHOLD)
		return createdPatterns;

	// SideToMove one pair from capture-win: always scan — a quiet last move must not
	// hide an immediate capture mate for the side to move.
	const bool sideToMoveNearCaptureWin = position.getCapturesForColor(sideToMove) >= 8;

	if (!sideToMoveNearCaptureWin
		&& createdPatterns > -THREAT_SCAN_THRESHOLD
		&& createdPatterns < THREAT_SCAN_THRESHOLD)
		return createdPatterns;

	const int sideToMoveBest = bestThreatNear(position, sideToMove, cell);
	return createdPatterns + signedFromAi(sideToMove, sideToMoveBest);
}

template <typename Traits>
int	MasterAI<Traits>::signedFromAi(Color side, int raw) const
{
	return (side == _aiColor) ? raw : -raw;
}

template <typename Traits>
int	MasterAI<Traits>::evaluatePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	int	result = 0;
	BitboardTool<Traits>& bitboardTool = BitboardTool<Traits>::instance();
	t_BWBoard<Traits> board = position.board();

	// makeMove already flipped sideToMove — the player who just placed at cell
	// is the OPPONENT of sideToMove().
	const Color side = (position.sideToMove() == Color::Black) ? Color::White : Color::Black;

	
	// ne verifie pas les captures gagnantes, seulement les alignements de 5
	if (isWinAfterMove<Traits>(board, side, cell.x, cell.y))
		return signedFromAi(side, 1000000);

	// n'enrigistre pas le compte de capture et ne modifie pas l'etat du board.
	// methode bientot obsolete, le nombre de capture sera enregistre dans le SearchPosition.
	const CaptureResult<Traits> caps = bitboardTool.resolveCaptures(board, cell.x, cell.y, side);
	if (caps.count)
		return signedFromAi(side, 200 * caps.count);

	if (side == Color::Black)
		result = bitboardTool.check_open_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_open_four(board.white, board.black, cell.x, cell.y);
	if (result == 2) // open four
		return signedFromAi(side, 500000);
	else if (result == 1) // half-open four
		return signedFromAi(side, 5000);

	if (side == Color::Black)
		result = bitboardTool.check_super_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_super_four(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, 60000);

	if (side == Color::Black)
		result = bitboardTool.check_broken_four(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_broken_four(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, 6000);

	if (side == Color::Black)
		result = bitboardTool.check_cross(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_cross(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, cross_score(result));

	if (side == Color::Black)
		result = bitboardTool.check_open_three(board.black, board.white, cell.x, cell.y);
	else
		result = bitboardTool.check_open_three(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(side, score_open_three(result));

	return 0;
}

template <typename Traits>
int MasterAI<Traits>::evaluateBlackPosition(
	const SearchPosition<Traits>& position,
	t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();
	
	const auto& board = position.board();

	// _whiteCaptures = stones Black has taken.
	const int totalByBlack = position.getTotalwhiteCaptures();
	const int capsThisMove = position.getWhiteCaptures();
	const int captureScore = captureProgressScore(totalByBlack - capsThisMove, capsThisMove);

	if (totalByBlack >= 10)
		return signedFromAi(Color::Black, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::Black, cell.x, cell.y))
		return signedFromAi(Color::Black, 1000000 + captureScore);

	int result = tool.check_open_four(board.black, board.white, cell.x, cell.y);
	if (result == 2)
		return signedFromAi(Color::Black, 500000 + captureScore);
	if (result == 1)
		return signedFromAi(Color::Black, 5000 + captureScore);

	if (tool.check_super_four(board.black, board.white, cell.x, cell.y))
		return signedFromAi(Color::Black, 60000 + captureScore);

	if (tool.check_broken_four(board.black, board.white, cell.x, cell.y))
		return signedFromAi(Color::Black, 6000 + captureScore);

	result = tool.check_cross(board.black, board.white, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::Black, cross_score(result) + captureScore);

	result = tool.check_open_three(board.black, board.white, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::Black, score_open_three(result) + captureScore);

	return signedFromAi(Color::Black, captureScore);
}


template <typename Traits>
int MasterAI<Traits>::evaluateWhitePosition(
	const SearchPosition<Traits>& position,
	t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();
	
	const auto& board = position.board();

	// _blackCaptures = stones White has taken.
	const int totalByWhite = position.getTotalblackCaptures();
	const int capsThisMove = position.getBlackCaptures();
	const int captureScore = captureProgressScore(totalByWhite - capsThisMove, capsThisMove);

	if (totalByWhite >= 10)
		return signedFromAi(Color::White, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::White, cell.x, cell.y))
		return signedFromAi(Color::White, 1000000 + captureScore);

	int result = tool.check_open_four(board.white, board.black, cell.x, cell.y);
	if (result == 2)
		return signedFromAi(Color::White, 500000 + captureScore);
	if (result == 1)
		return signedFromAi(Color::White, 5000 + captureScore);

	if (tool.check_super_four(board.white, board.black, cell.x, cell.y))
		return signedFromAi(Color::White, 60000 + captureScore);

	if (tool.check_broken_four(board.white, board.black, cell.x, cell.y))
		return signedFromAi(Color::White, 6000 + captureScore);

	result = tool.check_cross(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::White, cross_score(result) + captureScore);

	result = tool.check_open_three(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::White, score_open_three(result) + captureScore);

	return signedFromAi(Color::White, captureScore);
}