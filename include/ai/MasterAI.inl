#include "ai/MasterAI.hpp"
#include "game/turn/WinDetector.hpp"
#include "logger/Logger.hpp"

static constexpr int WIN_SCORE      = 1000000;
static constexpr int MATE_THRESHOLD = 900000;
static constexpr int CAPTURE_SCORE  = 202;

// Nulle : la prise qui casse un cinq porte le défenseur à CAPTURES_TO_WIN.
// Les deux victoires tombent sur le même coup, personne ne l'emporte.
static constexpr int DRAW_SCORE     = 0;

// Cinq réfutable (règle de la capture finale) : l'adversaire n'a qu'une seule
// réponse, la prise qui casse la ligne. C'est donc plus contraignant qu'un
// open four (500 000, parable par blocage OU par capture) sans être gagné —
// et sous MATE_THRESHOLD, pour que la feuille continue de scanner les menaces.
static constexpr int BREAKABLE_FIVE_SCORE = 600000;

// Ne scanner que si le dernier coup ≥ broken-four.
static constexpr int THREAT_SCAN_THRESHOLD  = 5000;
static constexpr int THREAT_SCAN_EARLY_EXIT = 90000; // double-three / cross / open-four
static constexpr int LEAF_SCAN_RADIUS      = 2;

// Ordonnancement lazy :
//   1) light sur tous les candidats
//   2) si depth >= FULL_ORDER_MIN_DEPTH → complétion full (cross+déf) seulement
//      sur le top du classement (ceux qu'on risque de chercher)
static constexpr int FULL_ORDER_MIN_DEPTH = 4;

// Forward pruning : nombre max de coups LÉGAUX réellement recherchés dans un
// nœud. Les coups étant triés best-first, les suivants sont abandonnés sans
// être vus — c'est donc lossy, un mauvais réglage peut faire manquer le
// meilleur coup.
// La limite dépend de la profondeur RESTANTE sous le nœud : près de la racine
// une erreur d'ordonnancement coûte tout le sous-arbre, on reste large ; près
// des feuilles les nœuds sont les plus nombreux et leur score le moins
// discriminant, on serre pour réduire le facteur de branchement effectif.
// Index = profondeur restante ; au-delà, la valeur de la racine.
// Table à tuner via le benchmark [PERF][BENCH].
static constexpr int MOVES_SEARCHED_BY_DEPTH[]   = { 4, 4, 6, 8, 10, 12, 14 };

// static constexpr int MOVES_SEARCHED_BY_DEPTH[]   = { 4, 6, 8, 10, 12, 14, 16 };
static constexpr int MOVES_SEARCHED_BY_DEPTH_LEN =
	static_cast<int>(sizeof(MOVES_SEARCHED_BY_DEPTH) / sizeof(MOVES_SEARCHED_BY_DEPTH[0]));

static constexpr int maxMovesSearched(int depth)
{
	return depth <= 0 ? MOVES_SEARCHED_BY_DEPTH[0]
	     : depth < MOVES_SEARCHED_BY_DEPTH_LEN ? MOVES_SEARCHED_BY_DEPTH[depth]
	     : MOVES_SEARCHED_BY_DEPTH[MOVES_SEARCHED_BY_DEPTH_LEN - 1];
}

// Marge au-dessus de maxMovesSearched pour le re-score full (si le light
// sous-classe un bon coup défensif juste hors du top-N).
static constexpr int LAZY_FULL_MARGIN = 4;

// Progressive race-to-10 incentive. `totalBefore` / `capsThisMove` are stone counts.
// Near the threshold this must rival half-open / broken fours so captures are planned.
inline int captureProgressScore(int totalBefore, int capsThisMove)
{
	const int totalAfter = totalBefore + capsThisMove;
	int score = capsThisMove * CAPTURE_SCORE * 2;
	// Quadratic progress: 2→160, 4→640, 6→1440, 8→2560
	score += totalAfter * totalAfter * 40;
	if (capsThisMove > 0)
		score += totalBefore * 100;
	// One pair from winning — treat like a forcing threat for ordering
	if (totalAfter >= 8 && capsThisMove > 0 && totalAfter < 10)
		score += 8000;
	return score;
}

// "depuis le nœud" -> "depuis la racine" (au probe TT)
// ply = currentDepth
static inline int ttScoreFromEntry(int score, int ply)
{
	if (score > MATE_THRESHOLD)
		return score - ply;
	if (score < -MATE_THRESHOLD)
		return score + ply;
	return score;
}

// "depuis la racine" -> "depuis le nœud" (au store TT)
// ply = currentDepth
static inline int ttScoreToEntry(int score, int ply)
{
	if (score > MATE_THRESHOLD)
		return score + ply;
	if (score < -MATE_THRESHOLD)
		return score - ply;

	// captures ? 
	return score;
}

static std::string signedScoreLabel(int score, const char* label)
{
	if (score > 0)
		return std::string(" [AI_") + label + "]";
	return std::string(" [OPP_") + label + "]";
}

static std::string scoreMacroLabel(int score)
{
	if (score == 0)
		return "";

	const int magnitude = (score < 0) ? -score : score;

	if (magnitude > MATE_THRESHOLD)
    {
        const int plies = WIN_SCORE - magnitude;
        const char* side = (score > 0) ? " [AI_WIN" : " [OPP_WIN";
        return std::string(side) + " mate in " + std::to_string(plies) + " ply]";
    }


	const char* label = nullptr;

	switch (magnitude)
	{
		case 1000000: label = "WIN"; break;
		case 500000:  label = "OPEN_FOUR"; break;
		case 5000:    label = "HALF_OPEN_FOUR"; break;
		case 60000:   label = "SUPER_FOUR"; break;
		case 6000:    label = "BROKEN_FOUR"; break;
		case 90000:   label = "CROSS_FULL|DOUBLE_FULL_FULL"; break;
		case 85000:   label = "DOUBLE_HOLE_FULL"; break;
		case 80000:   label = "DOUBLE_HOLE_HOLE"; break;
		case 9000:    label = "CROSS_FULL_OPP_INTERN|DOUBLE_FULL_FULL_INTERN"; break;
		case 8500:    label = "DOUBLE_HOLE_FULL_INTERN"; break;
		case 8000:    label = "DOUBLE_HOLE_HOLE_INTERN"; break;
		case 7500:    label = "DOUBLE_FULL_FULL_MIXED"; break;
		case 7000:    label = "DOUBLE_HOLE_FULL_MIXED"; break;
		case 6500:    label = "DOUBLE_HOLE_HOLE_MIXED"; break;
		case 2000:    label = "CROSS_DEMI_NO_MID"; break;
		case 1500:    label = "CROSS_DEMI_MID"; break;
		case 800:     label = "SCORE_3_FULL"; break;
		case 700:     label = "SCORE_3_HOLE"; break;
		case 500:     label = "SCORE_FULL_EXTERN"; break;
		case 450:     label = "SCORE_HOLE_EXTERN"; break;
		case 350:     label = "CROSS_DEMI_MID_OPP_EXTERN"; break;
		case 300:     label = "CROSS_DEMI_NO_OPP_EXTERN"; break;
		case 175:     label = "CROSS_DEMI_MID_OPP_INTERN"; break;
		case 150:     label = "CROSS_DEMI_NO_OPP_INTERN"; break;
		case 90:      label = "DOUBLE_FULL_FULL_INTERN2"; break;
		case 85:      label = "DOUBLE_HOLE_FULL_INTERN2"; break;
		case 80:      label = "DOUBLE_HOLE_HOLE_INTERN2"; break;
		case 40:      label = "SCORE_FULL_INTERN"; break;
		case 35:      label = "SCORE_HOLE_INTERN"; break;
		default: break;
	}

	if (label)
		return signedScoreLabel(score, label);
	
	const std::string prefix = (score > 0) ? " [AI_UNKNOWN:" : " [OPP_UNKNOWN:";
	return prefix + std::to_string(magnitude) + "]";
}

template <typename Traits>
MasterAI<Traits>::MasterAI(int depth, int activeZoneRadius, Color aiColor)
	: _maxDepth(depth), _aiColor(aiColor), _moveGenerator(activeZoneRadius)
{
	_stoneCapturedByAI = 0;
	_stoneCapturedByOPP = 0;
	resetOrderingHeuristics();
}

template <typename Traits>
void	MasterAI<Traits>::setSearchDepth(int depth) noexcept
{
	_maxDepth = depth;
}

template <typename Traits>
int	MasterAI<Traits>::getSearchDepth() const noexcept
{
	return _maxDepth;
}

template <typename Traits>
t_cell MasterAI<Traits>::tryToPlayEarlyOpeningMove(const t_BWBoard<Traits>& board) const noexcept
{
	const int stoneCount = popcount_bb_generic<Traits>(board.black)
	                     + popcount_bb_generic<Traits>(board.white);

	if (stoneCount == 0)
	{
		return {
			static_cast<int_fast16_t>(Traits::BOARD_SIZE / 2),
			static_cast<int_fast16_t>(Traits::BOARD_SIZE / 2)};
	}

	if (stoneCount == 1)
	{
		t_cell anchor{-1, -1};
		bb_for_each_bit<Traits>(board.black, [&](int x, int y) {
			anchor = {static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y)};
		});
		if (anchor.x < 0)
		{
			bb_for_each_bit<Traits>(board.white, [&](int x, int y) {
				anchor = {static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y)};
			});
		}

		static constexpr int kDx[8] = { 1,  1, -1, -1,  1,  0, -1,  0 };
		static constexpr int kDy[8] = { 1, -1,  1, -1,  0,  1,  0, -1 };
		for (int i = 0; i < 8; ++i)
		{
			const int x = anchor.x + kDx[i];
			const int y = anchor.y + kDy[i];
			if (!in_board_generic<Traits>(x, y))
				continue;
			if (get_bb_generic<Traits>(board.black, x, y)
				|| get_bb_generic<Traits>(board.white, x, y))
				continue;
			return {
				static_cast<int_fast16_t>(x),
				static_cast<int_fast16_t>(y)};
		}
	}

	return {-1, -1};
}

// Le coup `move` (déjà coté, donc son masque de capture est connu) aboutit-il à
// un cinq que l'adversaire ne peut pas casser ? La racine n'a pas de make/undo
// sous la main : on juge sur une copie du plateau après le coup.
template <typename Traits>
bool isUnbreakableFiveMove(const t_BWBoard<Traits>& board, const EvaluatedMove& move,
	Color color, int defenderCaptures)
{
	const t_BWBoard<Traits> next =
		board_after_move<Traits>(board, move.move.x, move.move.y, color, move.captureMask);

	return judgeFiveAfterMove<Traits>(next, color, move.move.x, move.move.y, defenderCaptures)
	       == FiveVerdict::Won;
}

template <typename Traits>
t_cell	MasterAI<Traits>::findBestMove(const SearchPosition<Traits>& position, Color color)
{
	_aiColor = color;

	_stats.nodesVisited   		= 0;
	_stats.nodesEvaluated 		= 0;
	_stats.nodesPruned   		= 0;
	_stats.maxDepthSeen  		= 0;
	_stats.ttHits          	= 0;
	_stats.ttCutoffs       	= 0;
	_stats.ttStores        	= 0;
	_stats.ttOrderingHits  	= 0;
	_stats.ttRootHits      	= 0;
	_stats.ttRootOrderingHits = 0;
	_stats.ttRootExactSeeds   = 0;
	_stats.forcedNodes        = 0;
	_stats.bestScore          = 0;
	_stats.bestMove           = {-1, -1};

	const t_cell earlyOpeningMove = tryToPlayEarlyOpeningMove(position.board());
	if (earlyOpeningMove.x >= 0)
	{
		_stats.bestMove  = earlyOpeningMove;
		_stats.bestScore = 0;
		LOG_DEBUG("AI", "[findBestMove] early opening move → ("
			+ std::to_string(earlyOpeningMove.x) + "," + std::to_string(earlyOpeningMove.y) + ")");
		return earlyOpeningMove;
	}

	resetOrderingHeuristics();

	MoveList<t_cell, MAX_BOARD_MOVES<Traits>> rootMoves;
	
	_moveGenerator.generateEmptyMoves(position.candidateMask(), rootMoves);
	
	LOG_DEBUG("AI", "[findBestMove] depth=" + std::to_string(_maxDepth)
	              + "  root candidates=" + std::to_string(rootMoves.size()));

	if (rootMoves.empty())
		return {-1, -1};

	t_cell bestMove = {-1, -1};

	// Cinq adverse déjà posé : la partie n'étant pas finie, il est forcément en
	// sursis et ce coup-ci est la seule occasion de le casser. La racine ne
	// reçoit pas l'état de sursis de GameController, on le relit du plateau.
	const std::optional<PendingWin> rootPending =
		findExistingFive<Traits>(position.board(), opponentOf(color));

	if (rootPending.has_value())
		LOG_DEBUG("AI", "[findBestMove] opponent five pending at ("
			+ std::to_string(rootPending->col) + "," + std::to_string(rootPending->row)
			+ ") — must be broken by capture this move");

	// Tri statique des coups racine : offense + défense/2 + captures.
	MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits>> orderedRoot;
	{
		const t_BWBoard<Traits>& board = position.board();

		const int capturesOfSideToMove = position.getCapturesForSideToMove();

		for (size_t i = 0; i < rootMoves.size(); ++i)
		{
			// generateEmptyMoves n'applique pas la règle du double-trois :
			// StandardRules (même verdict que submitMove) décide, on passe au suivant.
			if (!_moveGenerator.isLegalMove(board, rootMoves[i].x, rootMoves[i].y, color))
				continue;

			EvaluatedMove scoredMove = computeRawScoreMove(board, rootMoves[i], color, capturesOfSideToMove);

			if (scoredMove.isLegal)
			{
				// La dixième capture ne gagne que si l'adversaire n'a pas de cinq
				// en sursis : face à une ligne posée, seule une prise qui la
				// casse compte, et elle vaut nulle et non victoire. On laisse
				// alors la recherche trancher.
				if (!rootPending.has_value()
					&& capture_mask_count(scoredMove.captureMask) + capturesOfSideToMove >= CAPTURES_TO_WIN)
				{
					_stats.bestScore = WIN_SCORE - 1;
					_stats.bestMove  = scoredMove.move;
					return scoredMove.move;
				}

				// Un cinq ne vaut plus le raccourci qu'imprenable, et seulement
				// si l'adversaire n'a pas lui-même une ligne en sursis : elle se
				// résoudrait avant la nôtre. Sinon on laisse chercher.
				if (scoredMove.score >= WIN_SCORE && !rootPending.has_value()
					&& isUnbreakableFiveMove<Traits>(board, scoredMove, color,
						position.getCapturesForColor(opponentOf(color))))
				{
					_stats.bestScore = WIN_SCORE - 1;
					_stats.bestMove  = scoredMove.move;
					return scoredMove.move;
				}

				addDefenseToOrderingScore(scoredMove, board, color, position.getCapturesForColor(color == Color::Black ? Color::White : Color::Black));
				orderedRoot.push(scoredMove);
			}
		}
		if (filterForcedReplies(board, orderedRoot, color))
			++_stats.forcedNodes;
		std::sort(orderedRoot.begin(), orderedRoot.end(),
			[](const EvaluatedMove& a, const EvaluatedMove& b) { return a.score > b.score; });
	}

	// TT probe at the ROOT — used ONLY for move ordering / seeding. We never cut
	// early here (no LOWER/UPPER return): the root must return the actual best
	// move, not just a bound. If the entry carries a bestMove, search it first.
	const TTEntry* rootHit = _tt.probe(position.zobristHash());
	if (rootHit)
		++_stats.ttRootHits;
	if (rootHit && rootHit->bestMove.x >= 0)
	{
		for (size_t i = 0; i < orderedRoot.size(); ++i)
		{
			if (orderedRoot[i].move.x == rootHit->bestMove.x &&
			    orderedRoot[i].move.y == rootHit->bestMove.y)
			{
				std::rotate(orderedRoot.begin(),
				            orderedRoot.begin() + i,
				            orderedRoot.begin() + i + 1);
				++_stats.ttRootOrderingHits;
				break;
			}
		}
	}

	int bestScore = std::numeric_limits<int>::min();
	int alpha = std::numeric_limits<int>::min();
	const int beta = std::numeric_limits<int>::max();

	// An EXACT root hit seeds bestMove/bestScore ONLY if that move is still
	// among the legal ordered candidates (guards TT collisions / stale cells).
	if (rootHit && rootHit->flag == TTFlag::Exact && rootHit->depth >= _maxDepth
		&& rootHit->bestMove.x >= 0)
	{
		for (size_t i = 0; i < orderedRoot.size(); ++i)
		{
			if (orderedRoot[i].move.x == rootHit->bestMove.x &&
			    orderedRoot[i].move.y == rootHit->bestMove.y)
			{
				bestScore = ttScoreFromEntry(rootHit->score, 0);
				bestMove  = rootHit->bestMove;
				++_stats.ttRootExactSeeds;
				break;
			}
		}
	}

	const int rootMaxMoves = maxMovesSearched(_maxDepth);
	
	int legalRootSearched = 0;
	
	for (size_t i = 0; i < orderedRoot.size(); ++i)
	{
		if (legalRootSearched >= rootMaxMoves)
			break;

		const t_cell& move = orderedRoot[i].move;

		if (!_moveGenerator.isLegalMove(position.board(), move.x, move.y, color))
			continue;

		++legalRootSearched;

		SearchPosition<Traits> newPosition = position;

		MoveStateHash moveHash = newPosition.buildMoveHash(orderedRoot[i], newPosition.sideToMove());
		
		newPosition.makeMove(move.x, move.y, newPosition.sideToMove(), moveHash);

		int score = minimax(newPosition, move, _maxDepth - 1, alpha, beta, rootPending);

		const std::string marker = (score > bestScore) ? " ← best" : "";
		const std::string macro  = scoreMacroLabel(score);

		LOG_DEBUG("AI", "[findBestMove] move (" + std::to_string(move.x) + "," + std::to_string(move.y) + ")  score =  " + std::to_string(score) + macro + marker);

		if (score > bestScore)
		{
			bestScore = score;
			bestMove  = move;
		}

		if (bestScore >= WIN_SCORE - 10)
			break;

		alpha = std::max(alpha, bestScore);
	}

	_stats.bestScore = bestScore;
	_stats.bestMove  = bestMove;

	const int pruningPct = _stats.nodesVisited > 0
		? (_stats.nodesPruned * 100 / _stats.nodesVisited) : 0;

	LOG_DEBUG("AI", "[findBestMove] stats:"
		"  visited="   + std::to_string(_stats.nodesVisited)
		+ "  evaluated=" + std::to_string(_stats.nodesEvaluated)
		+ "  pruned="    + std::to_string(_stats.nodesPruned)
		+ " (" + std::to_string(pruningPct) + "%)"
		+ "  maxDepth="  + std::to_string(_stats.maxDepthSeen)
		+ "  forced="    + std::to_string(_stats.forcedNodes));
	LOG_SUPPRESS(_stats.nodesVisited, _stats.nodesEvaluated, _stats.nodesPruned, _stats.maxDepthSeen, pruningPct);
	LOG_DEBUG("AI", "[findBestMove] tt [minimax] stores=" + std::to_string(_stats.ttStores)
		+ " hits=" + std::to_string(_stats.ttHits)
		+ " cutoffs=" + std::to_string(_stats.ttCutoffs)
		+ " orderingHits=" + std::to_string(_stats.ttOrderingHits));
	LOG_DEBUG("AI", "[findBestMove] tt [root] hits=" + std::to_string(_stats.ttRootHits)
		+ " orderingHits=" + std::to_string(_stats.ttRootOrderingHits)
		+ " exactSeeds=" + std::to_string(_stats.ttRootExactSeeds));
	

	return bestMove;
}

template <typename Traits>
int MasterAI<Traits>::minimax(SearchPosition<Traits>& position, t_cell cell,
    int depth, int alpha, int beta, std::optional<PendingWin> pending)
{
    const int currentDepth = _maxDepth - depth;
    if (currentDepth > _stats.maxDepthSeen)
        _stats.maxDepthSeen = currentDepth;

    const uint64_t ttKey = position.zobristHash();
    const TTEntry* ttHit = _tt.probe(ttKey);
    if (ttHit && ttHit->depth >= depth)
    {
        ++_stats.ttHits;
        const int ttScore = ttScoreFromEntry(ttHit->score, currentDepth);
        if (ttHit->flag == TTFlag::Exact) {
            ++_stats.ttCutoffs;
            return ttScore;
        }
        if (ttHit->flag == TTFlag::LowerBound)
            alpha = std::max(alpha, ttScore);
        else if (ttHit->flag == TTFlag::UpperBound)
            beta = std::min(beta, ttScore);
        if (alpha >= beta) {
            ++_stats.ttCutoffs;
            return ttScore;
        }
    }

    ++_stats.nodesVisited;

	const Color mover = position.sideToMove();

    const Color lastPlayed = (mover == Color::Black)
                           ? Color::White : Color::Black;

    // Terminaison, dans l'ordre imposé par la règle de la capture finale
    // (l'ordre compte : une prise qui ne casse pas la ligne ne sauve pas le
    // défenseur, donc le cinq en sursis passe AVANT la dixième capture) :
    //   1. cinq laissé en sursis par `mover` au ply précédent :
    //        — s'il a survécu au dernier coup, `mover` gagne ;
    //        — s'il a été cassé par une prise qui porte `lastPlayed` à dix, nulle ;
    //   2. dixième capture de `lastPlayed` ;
    //   3. cinq créé par le dernier coup, s'il est imprenable (ou nul).
    // Le sursis reste une propriété du plateau et du camp au trait — donc du
    // hash — la TT n'a pas besoin d'une clé supplémentaire.
    const int mate = WIN_SCORE - currentDepth;

    std::optional<PendingWin> nextPending = std::nullopt;

    if (pending.has_value())
    {
        if (pendingFiveSurvives<Traits>(position.board(), *pending))
        {
            // pending->owner == mover : l'adversaire n'a pas cassé la ligne.
            ++_stats.nodesEvaluated;
            return (mover == _aiColor) ? mate : -mate;
        }
        // Ligne cassée par `lastPlayed`. Si cette prise l'a porté à dix, sa
        // victoire par capture et l'alignement s'annulent : nulle.
        if (position.getCapturesForColor(lastPlayed) >= CAPTURES_TO_WIN)
        {
            ++_stats.nodesEvaluated;
            return DRAW_SCORE;
        }
    }

    if (position.getCapturesForColor(lastPlayed) >= CAPTURES_TO_WIN)
    {
        ++_stats.nodesEvaluated;
        return (lastPlayed == _aiColor) ? mate : -mate;
    }

    // Le is_five_in_a_row garde le chemin coûteux (masques + réfutabilité) hors
    // du cas courant, où aucun cinq n'est posé.
    if (isWinAfterMove<Traits>(position.board(), lastPlayed, cell.x, cell.y))
    {
        const FiveVerdict verdict = judgeFiveAfterMove<Traits>(position.board(), lastPlayed,
            cell.x, cell.y, position.getCapturesForColor(mover));

        if (verdict == FiveVerdict::Won)
        {
            ++_stats.nodesEvaluated;
            return (lastPlayed == _aiColor) ? mate : -mate;
        }
        if (verdict == FiveVerdict::Draw)
        {
            // `mover` a une prise qui casse la ligne et le porte à dix : quoi
            // que fasse `lastPlayed` ensuite, la position vaut nulle.
            ++_stats.nodesEvaluated;
            return DRAW_SCORE;
        }
        // Cinq réfutable : la partie continue, `mover` a un coup pour le casser.
        nextPending = PendingWin{ lastPlayed,
            static_cast<int>(cell.x), static_cast<int>(cell.y) };
    }

    if (depth == 0)
    {
        ++_stats.nodesEvaluated;
        return evaluateLeafPosition(position, cell);
    }

    MoveList<t_cell, MAX_BOARD_MOVES<Traits>> movesArray;
    
	_moveGenerator.generateEmptyMoves(position.candidateMask(), movesArray);

    if (movesArray.empty())
    {
        ++_stats.nodesEvaluated;
        return evaluateLeafPosition(position, cell);
    }


    const int   ply   = currentDepth;
    int capturesBefore = position.getCapturesForSideToMove();
    const t_BWBoard<Traits>& board = position.board();

	
    auto isBetterMove = [this, ply, mover](const EvaluatedMove& a, const EvaluatedMove& b) {
        if (a.score != b.score)
            return a.score > b.score;
        const bool ak = isKillerMove(ply, a.move);
        const bool bk = isKillerMove(ply, b.move);
        if (ak != bk)
            return ak;
        return historyScore(mover, a.move) > historyScore(mover, b.move);
    };

    MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits>> ordered;
    {
        for (size_t i = 0; i < movesArray.size(); ++i)
		{
			EvaluatedMove scoredMove = computeLightScore(board, movesArray[i], mover, capturesBefore);
			if (scoredMove.isLegal)
				ordered.push(scoredMove);
		}

		if (filterForcedReplies(board, ordered, mover))
			++_stats.forcedNodes;

		
		std::sort(ordered.begin(), ordered.end(), isBetterMove);

		if (depth >= FULL_ORDER_MIN_DEPTH && !ordered.empty())
		{
			const size_t pool = std::min(ordered.size(),
				static_cast<size_t>(maxMovesSearched(depth) + LAZY_FULL_MARGIN));
			for (size_t i = 0; i < pool; ++i)
			{
				upgradeLightToFull(ordered[i], board, mover, capturesBefore);
				addDefenseToOrderingScore(ordered[i], board, mover,
					position.getCapturesForColor(mover == Color::Black ? Color::White : Color::Black));
			}
			std::sort(ordered.begin(), ordered.end(), isBetterMove);
		}
    }

    // 6.b Ordonnancement dynamique : si la TT porte un bestMove pour cette
    //     position (même issu d'une recherche moins profonde), on le place en
    //     TÊTE de la liste. Deux effets :
    //       - il est TOUJOURS exploré, en contournant le plafond top-N
    //         (maxMovesSearched) qui aurait pu l'écarter s'il était mal classé
    //         par le tri statique ;
    //       - le meilleur coup connu est essayé en premier → coupures
    //         alpha-beta plus précoces.
    //     ttHit a été sondé en début de fonction et reste valide ici (aucun
    //     store n'a eu lieu entre-temps). Même logique qu'à la racine.
    if (ttHit && ttHit->bestMove.x >= 0)
    {
        for (size_t i = 0; i < ordered.size(); ++i)
        {
            if (ordered[i].move.x == ttHit->bestMove.x &&
                ordered[i].move.y == ttHit->bestMove.y)
            {
                std::rotate(ordered.begin(),
                            ordered.begin() + i,
                            ordered.begin() + i + 1);
                ++_stats.ttOrderingHits;
                break;
            }
        }
    }

    // 7. sideToMove AVANT makeMove → pour undoMove
    const bool isMaximizing = (position.sideToMove() == _aiColor);
    const int alphaOrig = alpha;
    const int betaOrig  = beta;
    t_cell bestMove  = {-1, -1};
    int    bestEval  = isMaximizing
                     ? std::numeric_limits<int>::min()
                     : std::numeric_limits<int>::max();

    int legalMovesSearched = 0;
    const int nodeMaxMoves = maxMovesSearched(depth);
    for (size_t i = 0; i < ordered.size(); ++i)
    {
        // Plafond top-N : on a déjà exploré les N meilleurs coups légaux.
        if (legalMovesSearched >= nodeMaxMoves)
            break;
		if (!ordered[i].isLegal)
			continue;

        const t_cell&         move      = ordered[i].move;
        const MoveStateHash moveHash  = position.buildMoveHash(ordered[i], position.sideToMove());

        // Sauvegarde la couleur AVANT makeMove
        const Color colorBeforeMove = position.sideToMove();

        ++legalMovesSearched;

        position.makeMove(move.x, move.y, colorBeforeMove, moveHash);

        // Principal Variation Search : 1er coup en fenêtre pleine ; suivants
        // en null-window, re-recherche pleine seulement sur fail-high.
        // (LMR mesuré : régression nette sur captures — re-searches >> économies.)
        int eval;
        if (legalMovesSearched == 1)
        {
            eval = minimax(position, move, depth - 1, alpha, beta, nextPending);
        }
        else if (isMaximizing)
        {
            eval = minimax(position, move, depth - 1, alpha, alpha + 1, nextPending);
            if (eval > alpha && eval < beta)
                eval = minimax(position, move, depth - 1, alpha, beta, nextPending);
        }
        else
        {
            eval = minimax(position, move, depth - 1, beta - 1, beta, nextPending);
            if (eval < beta && eval > alpha)
                eval = minimax(position, move, depth - 1, alpha, beta, nextPending);
        }

        position.undoMove(move.x, move.y, colorBeforeMove);

        if (isMaximizing)
        {
            if (eval > bestEval) { bestEval = eval; bestMove = move; }
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                ++_stats.nodesPruned;
                recordCutoff(ply, move, depth, colorBeforeMove);
                break;
            }
        }
        else
        {
            if (eval < bestEval) { bestEval = eval; bestMove = move; }
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                ++_stats.nodesPruned;
                recordCutoff(ply, move, depth, colorBeforeMove);
                break;
            }
        }

    }

    if (legalMovesSearched == 0)
    {
        ++_stats.nodesEvaluated;
        return evaluateLeafPosition(position, cell);
    }

    TTFlag flag;
    if      (bestEval <= alphaOrig) flag = TTFlag::UpperBound;
    else if (bestEval >= betaOrig)  flag = TTFlag::LowerBound;
    else                            flag = TTFlag::Exact;

    _tt.store(position.zobristHash(), ttScoreToEntry(bestEval, currentDepth),
              depth, flag, {bestMove.x, bestMove.y});
    ++_stats.ttStores;

    return bestEval;
}

// --------------------------------------------------------------------------
// Ordonnancement dynamique : killer moves + history heuristic
// --------------------------------------------------------------------------

template <typename Traits>
void MasterAI<Traits>::resetOrderingHeuristics()
{
    for (int p = 0; p < MAX_SEARCH_PLY; ++p)
    {
        _killers[p][0] = t_cell{ -1, -1 };
        _killers[p][1] = t_cell{ -1, -1 };
    }
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < Traits::CELL_COUNT; ++i)
            _history[c][i] = 0;
}

template <typename Traits>
bool MasterAI<Traits>::isKillerMove(int ply, t_cell move) const
{
    if (ply < 0 || ply >= MAX_SEARCH_PLY)
        return false;
    return (_killers[ply][0].x == move.x && _killers[ply][0].y == move.y) ||
           (_killers[ply][1].x == move.x && _killers[ply][1].y == move.y);
}

template <typename Traits>
int MasterAI<Traits>::historyScore(Color mover, t_cell move) const
{
    const int idx = idx_generic<Traits>(move.x, move.y);
    return _history[(mover == Color::Black) ? 0 : 1][idx];
}

// Enregistre le coup responsable d'une coupure beta : il devient killer pour ce
// ply (schéma à deux emplacements, sans doublon) et son compteur history est
// bonifié de depth^2 (les coupures près de la racine pèsent davantage).
template <typename Traits>
void MasterAI<Traits>::recordCutoff(int ply, t_cell move, int depth, Color mover)
{
    if (ply >= 0 && ply < MAX_SEARCH_PLY)
    {
        if (!(_killers[ply][0].x == move.x && _killers[ply][0].y == move.y))
        {
            _killers[ply][1] = _killers[ply][0];
            _killers[ply][0] = move;
        }
    }
    const int idx = idx_generic<Traits>(move.x, move.y);
    _history[(mover == Color::Black) ? 0 : 1][idx] += depth * depth;
}


template <typename Traits>
EvaluatedMove MasterAI<Traits>::computeRawScoreMove(const t_BWBoard<Traits>& board, t_cell cell, Color color, int capturesBefore)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove eval {};
	eval.isLegal = true;
	eval.move = cell;
	eval.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps = capture_mask_count(eval.captureMask);

	const int captureScore = captureProgressScore(capturesBefore, caps);

	if (capturesBefore + caps >= 10)
	{
		eval.score = 1000000 + captureScore;
		return eval;
	}

	eval.score = captureScore + 89;
	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		eval.score = 1000000 + captureScore;
		eval.stage = ShapeStage::Terminal;
		return eval;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	if (r == 2)
	{
		eval.score = 500000 + captureScore;
		eval.stage = ShapeStage::OpenFour;
		return eval;
	}
	if (r == 1) {
		eval.score = 5000 + captureScore;
		eval.stage = ShapeStage::HalfFour;
		return eval;
	}

	// if (tool.check_super_four(own, opp, cell.x, cell.y))
	// {
	// 	eval.score = 60000 + captureScore;
	// 	return eval;
	// }
	if (tool.check_broken_four(own, opp, cell.x, cell.y))
	{
		eval.score = 6000 + captureScore;
		eval.stage = ShapeStage::BrokenFour;
		return eval;
	}

	r = tool.check_cross(own, opp, cell.x, cell.y);
	if (r) 
	{
		eval.score = cross_score(r) + captureScore;
		eval.stage = ShapeStage::Cross;
		// Une croix n'exempte pas le double-trois (contrairement au four).
		eval.isLegal = !(caps == 0 && tool.is_double_three_at(own, opp, cell.x, cell.y));
		return eval;
	}

	r = tool.check_open_three(own, opp, cell.x, cell.y);
	if (r)
	{
		eval.score = score_open_three(r) + captureScore;
		// Exemption = capture ON THIS MOVE only (not race progress).
		eval.isLegal = !(tool.isDoubleThreeScore(r) && caps == 0);
		return eval;
	}
	return eval;
}


// Clé « full » de référence. Le tri de minimax ne l'appelle plus : il part de
// computeLightScore et complète avec upgradeLightToFull, qui produit le même
// résultat pour un seul scan de motif au lieu de cinq. L'équivalence des deux
// chemins est verrouillée par [Ordering] light+upgrade ≡ V2.
template <typename Traits>
EvaluatedMove MasterAI<Traits>::rawShapeScoreV2(const t_BWBoard<Traits>& board, t_cell cell, Color color, int captureCount)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y); // pose hypothétique (copie locale)

	EvaluatedMove eval {};
	eval.isLegal = true;
	eval.move = cell;
	eval.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps = capture_mask_count(eval.captureMask);

	const int captureScore = captureProgressScore(captureCount, caps);

	if ((captureCount + caps) >= 10)
	{
		eval.score = 1000000 + captureScore;
		eval.stage = ShapeStage::Terminal;
		return eval;
	}

	eval.score = captureScore + 89;
	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		eval.score = 1000000 + captureScore;
		eval.stage = ShapeStage::Terminal;
		return eval;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	if (r == 2)
	{
		eval.score = 500000 + captureScore;
		eval.stage = ShapeStage::OpenFour;
		return eval;
	}
	if (r == 1) {
		eval.score = 5000 + captureScore;
		eval.stage = ShapeStage::HalfFour;
		return eval;
	}

	// if (tool.check_super_four(own, opp, cell.x, cell.y))
	// {
	// 	eval.score = 60000 + captureScore;
	// 	return eval;
	// }
	if (tool.check_broken_four(own, opp, cell.x, cell.y))
	{
		eval.score = 6000 + captureScore;
		eval.stage = ShapeStage::BrokenFour;
		return eval;
	}

	r = tool.check_cross(own, opp, cell.x, cell.y);
	if (r) 
	{
		eval.score = cross_score(r) + captureScore;
		eval.stage = ShapeStage::Cross;
		// Une croix n'exempte pas le double-trois (contrairement au four).
		eval.isLegal = !(caps == 0 && tool.is_double_three_at(own, opp, cell.x, cell.y));
		return eval;
	}

	r = tool.check_open_three(own, opp, cell.x, cell.y);
	if (r)
	{
		eval.score = score_open_three(r) + captureScore;
		// Exemption = capture ON THIS MOVE only (not race progress).
		eval.isLegal = !(tool.isDoubleThreeScore(r) && caps == 0);
		return eval;
	}
	return eval;
}

// Clé light pour nœuds profonds : même captures/légalité que V2, mais sans
// check_cross (coûteux et redondant avec open_three pour le tri).
// `stage` mémorise l'étage de sortie : c'est ce qui permet à upgradeLightToFull
// de savoir si le check_cross omis peut encore changer quelque chose.
template <typename Traits>
EvaluatedMove MasterAI<Traits>::computeLightScore(const t_BWBoard<Traits>& board, t_cell cell, Color color, int captureCount)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove eval {};
	eval.isLegal = true;
	eval.move = cell;
	eval.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps = capture_mask_count(eval.captureMask);
	const int captureScore = captureProgressScore(captureCount, caps);

	if ((captureCount + caps) >= 10)
	{
		eval.score = 1000000 + captureScore;
		eval.stage = ShapeStage::Terminal;
		return eval;
	}

	eval.score = captureScore + 89;
	if (tool.is_five_in_a_row(own, cell.x, cell.y))
	{
		eval.score = 1000000 + captureScore;
		eval.stage = ShapeStage::Terminal;
		return eval;
	}

	int r = tool.check_open_four(own, opp, cell.x, cell.y);
	if (r == 2)
	{
		eval.score = 500000 + captureScore;
		eval.stage = ShapeStage::OpenFour;
		return eval;
	}
	if (r == 1)
	{
		eval.score = 5000 + captureScore;
		eval.stage = ShapeStage::HalfFour;
		return eval;
	}

	if (tool.check_broken_four(own, opp, cell.x, cell.y))
	{
		eval.score = 6000 + captureScore;
		eval.stage = ShapeStage::BrokenFour;
		return eval;
	}

	r = tool.check_open_three(own, opp, cell.x, cell.y);
	if (r)
	{
		eval.score = score_open_three(r) + captureScore;
		eval.isLegal = !(tool.isDoubleThreeScore(r) && !caps);
		return eval;
	}
	return eval;
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

// Si l'adversaire aligne cinq au coup suivant, seules trois familles de réponses
// peuvent encore changer l'issue — gagner immédiatement, occuper la case du cinq,
// ou capturer (une prise casse l'alignement, et la dixième pierre gagne). Tout le
// reste perd, et les explorer ne fait que gonfler le facteur de branchement là où
// l'arbre est le plus profond.
//
// La règle de la capture finale n'élargit pas cet ensemble : casser un cinq déjà
// posé est une prise, et toutes les prises sont conservées ici. Le filtre reste
// donc un sur-ensemble des parades valides, comme avant.
template <typename Traits>
bool MasterAI<Traits>::filterForcedReplies(const t_BWBoard<Traits>& board,
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
	const EvaluatedMove defense = computeRawScoreMove(board, offense.move, opp, oppCapturesBefore);
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
			const EvaluatedMove offense = computeRawScoreMove(board, cand, color, capsBefore);
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

	if (totalByBlack >= CAPTURES_TO_WIN)
		return signedFromAi(Color::Black, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::Black, cell.x, cell.y))
	{
		// Un cinq que Blanc peut casser à la prise ne vaut pas une victoire ; s'il
		// le casse en atteignant dix pierres, la position est nulle.
		const FiveVerdict verdict = judgeFiveAfterMove<Traits>(board, Color::Black,
			cell.x, cell.y, position.getCapturesForColor(Color::White));

		if (verdict == FiveVerdict::Draw)
			return DRAW_SCORE;

		return signedFromAi(Color::Black,
			(verdict == FiveVerdict::Won ? 1000000 : BREAKABLE_FIVE_SCORE) + captureScore);
	}

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

	if (totalByWhite >= CAPTURES_TO_WIN)
		return signedFromAi(Color::White, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::White, cell.x, cell.y))
	{
		// Un cinq que Noir peut casser à la prise ne vaut pas une victoire ; s'il
		// le casse en atteignant dix pierres, la position est nulle.
		const FiveVerdict verdict = judgeFiveAfterMove<Traits>(board, Color::White,
			cell.x, cell.y, position.getCapturesForColor(Color::Black));

		if (verdict == FiveVerdict::Draw)
			return DRAW_SCORE;

		return signedFromAi(Color::White,
			(verdict == FiveVerdict::Won ? 1000000 : BREAKABLE_FIVE_SCORE) + captureScore);
	}

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
