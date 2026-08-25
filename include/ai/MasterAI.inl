#include "ai/MasterAI.hpp"
#include "game/turn/WinDetector.hpp"
#include "logger/Logger.hpp"

static constexpr int WIN_SCORE      = 1000000;
static constexpr int MATE_THRESHOLD = 900000;
static constexpr int CAPTURE_SCORE  = 202;

static constexpr int DRAW_SCORE = 0;

static constexpr int BREAKABLE_FIVE_SCORE = 600000;

static constexpr int THREAT_SCAN_THRESHOLD  = 5000;
static constexpr int THREAT_SCAN_EARLY_EXIT = 90000; // double-three / cross / open-four
static constexpr int LEAF_SCAN_RADIUS       = 2;

static constexpr int FULL_ORDER_MIN_DEPTH = 4;

static constexpr int MOVES_SEARCHED_BY_DEPTH[] = { 4, 4, 6, 8, 10, 12, 14 };

static constexpr int MOVES_SEARCHED_BY_DEPTH_LEN =
	static_cast<int>(sizeof(MOVES_SEARCHED_BY_DEPTH) / sizeof(MOVES_SEARCHED_BY_DEPTH[0]));

static constexpr int maxMovesSearched(int depth)
{
	return depth <= 0                            ? MOVES_SEARCHED_BY_DEPTH[0]
	       : depth < MOVES_SEARCHED_BY_DEPTH_LEN ? MOVES_SEARCHED_BY_DEPTH[depth]
	                                             : MOVES_SEARCHED_BY_DEPTH[MOVES_SEARCHED_BY_DEPTH_LEN - 1];
}

static constexpr int LAZY_FULL_MARGIN = 4;

inline int captureProgressScore(int totalBefore, int capsThisMove)
{
	const int totalAfter = totalBefore + capsThisMove;
	int       score      = capsThisMove * CAPTURE_SCORE * 2;
	// Quadratic progress: 2→160, 4→640, 6→1440, 8→2560
	score += totalAfter * totalAfter * 40;
	if (capsThisMove > 0)
		score += totalBefore * 100;
	// One pair from winning
	if (totalAfter >= 8 && capsThisMove > 0 && totalAfter < 10)
		score += 8000;
	return score;
}

// ply = currentDepth
static inline int ttScoreFromEntry(int score, int ply)
{
	if (score > MATE_THRESHOLD)
		return score - ply;
	if (score < -MATE_THRESHOLD)
		return score + ply;
	return score;
}

static inline int ttScoreToEntry(int score, int ply)
{
	if (score > MATE_THRESHOLD)
		return score + ply;
	if (score < -MATE_THRESHOLD)
		return score - ply;

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
		const int   plies = WIN_SCORE - magnitude;
		const char* side  = (score > 0) ? " [AI_WIN" : " [OPP_WIN";
		return std::string(side) + " mate in " + std::to_string(plies) + " ply]";
	}

	const char* label = nullptr;

	switch (magnitude)
	{
		case 1000000:
			label = "WIN";
			break;
		case 500000:
			label = "OPEN_FOUR";
			break;
		case 5000:
			label = "HALF_OPEN_FOUR";
			break;
		case 60000:
			label = "SUPER_FOUR";
			break;
		case 6000:
			label = "BROKEN_FOUR";
			break;
		case 90000:
			label = "CROSS_FULL|DOUBLE_FULL_FULL";
			break;
		case 85000:
			label = "DOUBLE_HOLE_FULL";
			break;
		case 80000:
			label = "DOUBLE_HOLE_HOLE";
			break;
		case 9000:
			label = "CROSS_FULL_OPP_INTERN|DOUBLE_FULL_FULL_INTERN";
			break;
		case 8500:
			label = "DOUBLE_HOLE_FULL_INTERN";
			break;
		case 8000:
			label = "DOUBLE_HOLE_HOLE_INTERN";
			break;
		case 7500:
			label = "DOUBLE_FULL_FULL_MIXED";
			break;
		case 7000:
			label = "DOUBLE_HOLE_FULL_MIXED";
			break;
		case 6500:
			label = "DOUBLE_HOLE_HOLE_MIXED";
			break;
		case 2000:
			label = "CROSS_DEMI_NO_MID";
			break;
		case 1500:
			label = "CROSS_DEMI_MID";
			break;
		case 800:
			label = "SCORE_3_FULL";
			break;
		case 700:
			label = "SCORE_3_HOLE";
			break;
		case 500:
			label = "SCORE_FULL_EXTERN";
			break;
		case 450:
			label = "SCORE_HOLE_EXTERN";
			break;
		case 350:
			label = "CROSS_DEMI_MID_OPP_EXTERN";
			break;
		case 300:
			label = "CROSS_DEMI_NO_OPP_EXTERN";
			break;
		case 175:
			label = "CROSS_DEMI_MID_OPP_INTERN";
			break;
		case 150:
			label = "CROSS_DEMI_NO_OPP_INTERN";
			break;
		case 90:
			label = "DOUBLE_FULL_FULL_INTERN2";
			break;
		case 85:
			label = "DOUBLE_HOLE_FULL_INTERN2";
			break;
		case 80:
			label = "DOUBLE_HOLE_HOLE_INTERN2";
			break;
		case 40:
			label = "SCORE_FULL_INTERN";
			break;
		case 35:
			label = "SCORE_HOLE_INTERN";
			break;
		default:
			break;
	}

	if (label)
		return signedScoreLabel(score, label);

	const std::string prefix = (score > 0) ? " [AI_UNKNOWN:" : " [OPP_UNKNOWN:";
	return prefix + std::to_string(magnitude) + "]";
}

template<typename Traits>
MasterAI<Traits>::MasterAI(int depth, int activeZoneRadius, Color aiColor)
	: _maxDepth(depth), _aiColor(aiColor), _moveGenerator(activeZoneRadius)
{
	_stoneCapturedByAI  = 0;
	_stoneCapturedByOPP = 0;
	resetOrderingHeuristics();
}

template<typename Traits> void MasterAI<Traits>::setSearchDepth(int depth) noexcept
{
	_maxDepth = depth;
}

template<typename Traits> int MasterAI<Traits>::getSearchDepth() const noexcept
{
	return _maxDepth;
}

template<typename Traits>
t_cell MasterAI<Traits>::tryToPlayEarlyOpeningMove(const t_BWBoard<Traits>& board) const noexcept
{
	const int stoneCount = popcount_bb_generic<Traits>(board.black) + popcount_bb_generic<Traits>(board.white);

	if (stoneCount == 0)
	{
		return { static_cast<int_fast16_t>(Traits::BOARD_SIZE / 2), static_cast<int_fast16_t>(Traits::BOARD_SIZE / 2) };
	}

	if (stoneCount == 1)
	{
		t_cell anchor{ -1, -1 };
		bb_for_each_bit<Traits>(board.black, [&](int x, int y)
		                        { anchor = { static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y) }; });
		if (anchor.x < 0)
		{
			bb_for_each_bit<Traits>(board.white, [&](int x, int y)
			                        { anchor = { static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y) }; });
		}

		static constexpr int kDx[8] = { 1, 1, -1, -1, 1, 0, -1, 0 };
		static constexpr int kDy[8] = { 1, -1, 1, -1, 0, 1, 0, -1 };
		for (int i = 0; i < 8; ++i)
		{
			const int x = anchor.x + kDx[i];
			const int y = anchor.y + kDy[i];
			if (!in_board_generic<Traits>(x, y))
				continue;
			if (get_bb_generic<Traits>(board.black, x, y) || get_bb_generic<Traits>(board.white, x, y))
				continue;
			return { static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y) };
		}
	}

	return { -1, -1 };
}

template<typename Traits>
bool isUnbreakableFiveMove(const t_BWBoard<Traits>& board, const EvaluatedMove& move, Color color, int defenderCaptures)
{
	const t_BWBoard<Traits> next = board_after_move<Traits>(board, move.move.x, move.move.y, color, move.captureMask);

	return judgeFiveAfterMove<Traits>(next, color, move.move.x, move.move.y, defenderCaptures) == FiveVerdict::Won;
}

template<typename Traits> t_cell MasterAI<Traits>::findBestMove(const SearchPosition<Traits>& position, Color color)
{
	_aiColor = color;

	_stats.nodesVisited       = 0;
	_stats.nodesEvaluated     = 0;
	_stats.nodesPruned        = 0;
	_stats.maxDepthSeen       = 0;
	_stats.ttHits             = 0;
	_stats.ttCutoffs          = 0;
	_stats.ttStores           = 0;
	_stats.ttOrderingHits     = 0;
	_stats.ttRootHits         = 0;
	_stats.ttRootOrderingHits = 0;
	_stats.ttRootExactSeeds   = 0;
	_stats.forcedNodes        = 0;
	_stats.bestScore          = 0;
	_stats.bestMove           = { -1, -1 };

	const t_cell earlyOpeningMove = tryToPlayEarlyOpeningMove(position.board());
	if (earlyOpeningMove.x >= 0)
	{
		_stats.bestMove  = earlyOpeningMove;
		_stats.bestScore = 0;
		LOG_DEBUG("AI", "[findBestMove] early opening move → (" + std::to_string(earlyOpeningMove.x) + "," +
		                    std::to_string(earlyOpeningMove.y) + ")");
		return earlyOpeningMove;
	}

	resetOrderingHeuristics();

	MoveList<t_cell, MAX_BOARD_MOVES<Traits> > rootMoves;

	_moveGenerator.generateEmptyMoves(position.candidateMask(), rootMoves);

	LOG_DEBUG("AI", "[findBestMove] depth=" + std::to_string(_maxDepth) +
	                    "  root candidates=" + std::to_string(rootMoves.size()));

	if (rootMoves.empty())
		return { -1, -1 };

	t_cell bestMove = { -1, -1 };

	const std::optional<PendingWin> rootPending = findExistingFive<Traits>(position.board(), opponentOf(color));

	if (rootPending.has_value())
		LOG_DEBUG("AI", "[findBestMove] opponent five pending at (" + std::to_string(rootPending->col) + "," +
		                    std::to_string(rootPending->row) + ") — must be broken by capture this move");

	// Tri statique des coups racine : offense + défense/2 + captures.
	MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits> > orderedRoot;
	{
		const t_BWBoard<Traits>& board = position.board();

		const int capturesOfSideToMove = position.getCapturesForSideToMove();

		for (size_t i = 0; i < rootMoves.size(); ++i)
		{
			EvaluatedMove scoredMove = computeRawScoreMove(board, rootMoves[i], color, capturesOfSideToMove);

			if (scoredMove.isLegal)
			{
				if (!rootPending.has_value() &&
				    capture_mask_count(scoredMove.captureMask) + capturesOfSideToMove >= CAPTURES_TO_WIN)
				{
					_stats.bestScore = WIN_SCORE - 1;
					_stats.bestMove  = scoredMove.move;
					return scoredMove.move;
				}

				if (scoredMove.score >= WIN_SCORE && !rootPending.has_value() &&
				    isUnbreakableFiveMove<Traits>(board, scoredMove, color,
				                                  position.getCapturesForColor(opponentOf(color))))
				{
					_stats.bestScore = WIN_SCORE - 1;
					_stats.bestMove  = scoredMove.move;
					return scoredMove.move;
				}

				addDefenseToOrderingScore(
					scoredMove, board, color,
					position.getCapturesForColor(color == Color::Black ? Color::White : Color::Black));
				orderedRoot.push(scoredMove);
			}
		}
		if (filterForcedReplies(board, orderedRoot, color))
			++_stats.forcedNodes;
		std::sort(orderedRoot.begin(), orderedRoot.end(),
		          [](const EvaluatedMove& a, const EvaluatedMove& b) { return a.score > b.score; });
	}

	const TTEntry* rootHit = _tt.probe(position.zobristHash());
	if (rootHit)
		++_stats.ttRootHits;
	if (rootHit && rootHit->bestMove.x >= 0)
	{
		for (size_t i = 0; i < orderedRoot.size(); ++i)
		{
			if (orderedRoot[i].move.x == rootHit->bestMove.x && orderedRoot[i].move.y == rootHit->bestMove.y)
			{
				std::rotate(orderedRoot.begin(), orderedRoot.begin() + i, orderedRoot.begin() + i + 1);
				++_stats.ttRootOrderingHits;
				break;
			}
		}
	}

	int       bestScore = std::numeric_limits<int>::min();
	int       alpha     = std::numeric_limits<int>::min();
	const int beta      = std::numeric_limits<int>::max();

	if (rootHit && rootHit->flag == TTFlag::Exact && rootHit->depth >= _maxDepth && rootHit->bestMove.x >= 0)
	{
		for (size_t i = 0; i < orderedRoot.size(); ++i)
		{
			if (orderedRoot[i].move.x == rootHit->bestMove.x && orderedRoot[i].move.y == rootHit->bestMove.y)
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

		++legalRootSearched;

		SearchPosition<Traits> newPosition = position;

		MoveStateHash moveHash = newPosition.buildMoveHash(orderedRoot[i], newPosition.sideToMove());

		newPosition.makeMove(move.x, move.y, newPosition.sideToMove(), moveHash);

		int score = minimax(newPosition, move, _maxDepth - 1, alpha, beta, rootPending);

		const std::string marker = (score > bestScore) ? " ← best" : "";
		const std::string macro  = scoreMacroLabel(score);

		LOG_DEBUG("AI", "[findBestMove] move (" + std::to_string(move.x) + "," + std::to_string(move.y) +
		                    ")  score =  " + std::to_string(score) + macro + marker);

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

	const int pruningPct = _stats.nodesVisited > 0 ? (_stats.nodesPruned * 100 / _stats.nodesVisited) : 0;

	LOG_DEBUG("AI", "[findBestMove] stats:"
	                "  visited=" +
	                    std::to_string(_stats.nodesVisited) + "  evaluated=" + std::to_string(_stats.nodesEvaluated) +
	                    "  pruned=" + std::to_string(_stats.nodesPruned) + " (" + std::to_string(pruningPct) + "%)" +
	                    "  maxDepth=" + std::to_string(_stats.maxDepthSeen) +
	                    "  forced=" + std::to_string(_stats.forcedNodes));
	LOG_SUPPRESS(_stats.nodesVisited, _stats.nodesEvaluated, _stats.nodesPruned, _stats.maxDepthSeen, pruningPct);
	LOG_DEBUG("AI", "[findBestMove] tt [minimax] stores=" + std::to_string(_stats.ttStores) +
	                    " hits=" + std::to_string(_stats.ttHits) + " cutoffs=" + std::to_string(_stats.ttCutoffs) +
	                    " orderingHits=" + std::to_string(_stats.ttOrderingHits));
	LOG_DEBUG("AI", "[findBestMove] tt [root] hits=" + std::to_string(_stats.ttRootHits) +
	                    " orderingHits=" + std::to_string(_stats.ttRootOrderingHits) +
	                    " exactSeeds=" + std::to_string(_stats.ttRootExactSeeds));

	return bestMove;
}

template<typename Traits>
int MasterAI<Traits>::minimax(SearchPosition<Traits>& position, t_cell cell, int depth, int alpha, int beta,
                              std::optional<PendingWin> pending)
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
		if (ttHit->flag == TTFlag::Exact)
		{
			++_stats.ttCutoffs;
			return ttScore;
		}
		if (ttHit->flag == TTFlag::LowerBound)
			alpha = std::max(alpha, ttScore);
		else if (ttHit->flag == TTFlag::UpperBound)
			beta = std::min(beta, ttScore);
		if (alpha >= beta)
		{
			++_stats.ttCutoffs;
			return ttScore;
		}
	}

	++_stats.nodesVisited;

	const Color mover = position.sideToMove();

	const Color lastPlayed = (mover == Color::Black) ? Color::White : Color::Black;

	const int mate = WIN_SCORE - currentDepth;

	std::optional<PendingWin> nextPending = std::nullopt;

	if (pending.has_value())
	{
		if (pendingFiveSurvives<Traits>(position.board(), *pending))
		{
			++_stats.nodesEvaluated;
			return (mover == _aiColor) ? mate : -mate;
		}
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

	if (isWinAfterMove<Traits>(position.board(), lastPlayed, cell.x, cell.y))
	{
		const FiveVerdict verdict = judgeFiveAfterMove<Traits>(position.board(), lastPlayed, cell.x, cell.y,
		                                                       position.getCapturesForColor(mover));

		if (verdict == FiveVerdict::Won)
		{
			++_stats.nodesEvaluated;
			return (lastPlayed == _aiColor) ? mate : -mate;
		}
		if (verdict == FiveVerdict::Draw)
		{
			++_stats.nodesEvaluated;
			return DRAW_SCORE;
		}
		nextPending = PendingWin{ lastPlayed, static_cast<int>(cell.x), static_cast<int>(cell.y) };
	}

	if (depth == 0)
	{
		++_stats.nodesEvaluated;
		return evaluateLeafPosition(position, cell);
	}

	MoveList<t_cell, MAX_BOARD_MOVES<Traits> > movesArray;

	_moveGenerator.generateEmptyMoves(position.candidateMask(), movesArray);

	if (movesArray.empty())
	{
		++_stats.nodesEvaluated;
		return evaluateLeafPosition(position, cell);
	}

	const int                ply            = currentDepth;
	int                      capturesBefore = position.getCapturesForSideToMove();
	const t_BWBoard<Traits>& board          = position.board();

	auto isBetterMove = [this, ply, mover](const EvaluatedMove& a, const EvaluatedMove& b)
	{
		if (a.score != b.score)
			return a.score > b.score;
		const bool ak = isKillerMove(ply, a.move);
		const bool bk = isKillerMove(ply, b.move);
		if (ak != bk)
			return ak;
		return historyScore(mover, a.move) > historyScore(mover, b.move);
	};

	MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits> > ordered;
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
			const size_t pool =
				std::min(ordered.size(), static_cast<size_t>(maxMovesSearched(depth) + LAZY_FULL_MARGIN));
			for (size_t i = 0; i < pool; ++i)
			{
				upgradeLightToFull(ordered[i], board, mover, capturesBefore);
				addDefenseToOrderingScore(
					ordered[i], board, mover,
					position.getCapturesForColor(mover == Color::Black ? Color::White : Color::Black));
			}
			std::sort(ordered.begin(), ordered.end(), isBetterMove);
		}
	}

	if (ttHit && ttHit->bestMove.x >= 0)
	{
		for (size_t i = 0; i < ordered.size(); ++i)
		{
			if (ordered[i].move.x == ttHit->bestMove.x && ordered[i].move.y == ttHit->bestMove.y)
			{
				std::rotate(ordered.begin(), ordered.begin() + i, ordered.begin() + i + 1);
				++_stats.ttOrderingHits;
				break;
			}
		}
	}

	const bool isMaximizing = (position.sideToMove() == _aiColor);
	const int  alphaOrig    = alpha;
	const int  betaOrig     = beta;
	t_cell     bestMove     = { -1, -1 };
	int        bestEval     = isMaximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

	int       legalMovesSearched = 0;
	const int nodeMaxMoves       = maxMovesSearched(depth);
	for (size_t i = 0; i < ordered.size(); ++i)
	{
		// Plafond top-N : on a déjà exploré les N meilleurs coups légaux.
		if (legalMovesSearched >= nodeMaxMoves)
			break;
		if (!ordered[i].isLegal)
			continue;

		const t_cell&       move     = ordered[i].move;
		const MoveStateHash moveHash = position.buildMoveHash(ordered[i], position.sideToMove());

		// Sauvegarde la couleur AVANT makeMove
		const Color colorBeforeMove = position.sideToMove();

		++legalMovesSearched;

		position.makeMove(move.x, move.y, colorBeforeMove, moveHash);

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
			if (eval > bestEval)
			{
				bestEval = eval;
				bestMove = move;
			}
			alpha = std::max(alpha, eval);
			if (beta <= alpha)
			{
				++_stats.nodesPruned;
				recordCutoff(ply, move, depth, colorBeforeMove);
				break;
			}
		}
		else
		{
			if (eval < bestEval)
			{
				bestEval = eval;
				bestMove = move;
			}
			beta = std::min(beta, eval);
			if (beta <= alpha)
			{
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
	if (bestEval <= alphaOrig)
		flag = TTFlag::UpperBound;
	else if (bestEval >= betaOrig)
		flag = TTFlag::LowerBound;
	else
		flag = TTFlag::Exact;

	_tt.store(position.zobristHash(), ttScoreToEntry(bestEval, currentDepth), depth, flag, { bestMove.x, bestMove.y });
	++_stats.ttStores;

	return bestEval;
}

// --------------------------------------------------------------------------
// Ordonnancement dynamique : killer moves + history heuristic
// --------------------------------------------------------------------------

template<typename Traits> void MasterAI<Traits>::resetOrderingHeuristics()
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

template<typename Traits> bool MasterAI<Traits>::isKillerMove(int ply, t_cell move) const
{
	if (ply < 0 || ply >= MAX_SEARCH_PLY)
		return false;
	return (_killers[ply][0].x == move.x && _killers[ply][0].y == move.y) ||
	       (_killers[ply][1].x == move.x && _killers[ply][1].y == move.y);
}

template<typename Traits> int MasterAI<Traits>::historyScore(Color mover, t_cell move) const
{
	const int idx = idx_generic<Traits>(move.x, move.y);
	return _history[(mover == Color::Black) ? 0 : 1][idx];
}

// Enregistre le coup responsable d'une coupure beta
template<typename Traits> void MasterAI<Traits>::recordCutoff(int ply, t_cell move, int depth, Color mover)
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

template<typename Traits>
EvaluatedMove MasterAI<Traits>::computeRawScoreMove(const t_BWBoard<Traits>& board, t_cell cell, Color color,
                                                    int capturesBefore)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard        own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove eval{};
	eval.isLegal     = true;
	eval.move        = cell;
	eval.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps   = capture_mask_count(eval.captureMask);

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

	// Le three passe AVANT le cross : seul un four dispense de la regle du double-trois
	const int three = tool.check_open_three(own, opp, cell.x, cell.y);
	if (three)
	{
		eval.score = score_open_three(three) + captureScore;
		eval.isLegal = !(tool.isDoubleThreeScore(three) && caps == 0);
		if (!eval.isLegal)
			return eval;
	}

	const int cross = tool.check_cross(own, opp, cell.x, cell.y);
	if (cross)
	{
		const int crossScore = cross_score(cross) + captureScore;
		if (crossScore > eval.score)
		{
			eval.score = crossScore;
			eval.stage = ShapeStage::Cross;
		}
	}
	return eval;
}

template<typename Traits>
EvaluatedMove MasterAI<Traits>::rawShapeScoreV2(const t_BWBoard<Traits>& board, t_cell cell, Color color,
                                                int captureCount)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard        own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove eval{};
	eval.isLegal     = true;
	eval.move        = cell;
	eval.captureMask = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps   = capture_mask_count(eval.captureMask);

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

	const int three = tool.check_open_three(own, opp, cell.x, cell.y);
	if (three)
	{
		eval.score = score_open_three(three) + captureScore;
		eval.isLegal = !(tool.isDoubleThreeScore(three) && caps == 0);
		if (!eval.isLegal)
			return eval;
	}

	const int cross = tool.check_cross(own, opp, cell.x, cell.y);
	if (cross)
	{
		const int crossScore = cross_score(cross) + captureScore;
		if (crossScore > eval.score)
		{
			eval.score = crossScore;
			eval.stage = ShapeStage::Cross;
		}
	}
	return eval;
}

template<typename Traits>
EvaluatedMove MasterAI<Traits>::computeLightScore(const t_BWBoard<Traits>& board, t_cell cell, Color color,
                                                  int captureCount)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard        own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, cell.x, cell.y);

	EvaluatedMove eval{};
	eval.isLegal           = true;
	eval.move              = cell;
	eval.captureMask       = detect_capture_mask(board, cell.x, cell.y, color);
	const int caps         = capture_mask_count(eval.captureMask);
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
		eval.score   = score_open_three(r) + captureScore;
		eval.isLegal = !(tool.isDoubleThreeScore(r) && !caps);
		return eval;
	}
	return eval;
}

template<typename Traits>
void MasterAI<Traits>::upgradeLightToFull(EvaluatedMove& move, const t_BWBoard<Traits>& board, Color color,
                                          int captureCount)
{
	if (move.stage != ShapeStage::ThreeOrQuiet || !move.isLegal)
		return;

	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	typename Traits::Bitboard        own = (color == Color::Black) ? board.black : board.white;
	const typename Traits::Bitboard& opp = (color == Color::Black) ? board.white : board.black;

	set_bb_generic<Traits>(own, move.move.x, move.move.y);

	const int r = tool.check_cross(own, opp, move.move.x, move.move.y);
	if (!r)
		return;

	const int caps       = capture_mask_count(move.captureMask);
	const int crossScore = cross_score(r) + captureProgressScore(captureCount, caps);
	if (crossScore > move.score)
	{
		move.score = crossScore;
		move.stage = ShapeStage::Cross;
	}
}

template<typename Traits>
bool MasterAI<Traits>::filterForcedReplies(const t_BWBoard<Traits>&                           board,
                                           MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits> >& ordered, Color mover)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	const Color                      oppColor  = (mover == Color::Black) ? Color::White : Color::Black;
	const typename Traits::Bitboard& oppStones = bitboardForColor(board, oppColor);

	bool isBlock[MAX_BOARD_MOVES<Traits>] = {};
	bool threatened                       = false;

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

	if (kept == 0)
		return false;

	ordered.count = kept;
	return true;
}

template<typename Traits>
void MasterAI<Traits>::addDefenseToOrderingScore(EvaluatedMove& offense, const t_BWBoard<Traits>& board, Color side,
                                                 int oppCapturesBefore)
{
	const Color         opp     = (side == Color::Black) ? Color::White : Color::Black;
	const EvaluatedMove defense = computeRawScoreMove(board, offense.move, opp, oppCapturesBefore);
	offense.score += defense.score / 2;
}

template<typename Traits>
int MasterAI<Traits>::bestThreatNear(const SearchPosition<Traits>& position, Color color, t_cell anchor)
{
	const t_BWBoard<Traits>& board      = position.board();
	const auto               zone       = position.candidateMask();
	const int                capsBefore = position.getCapturesForColor(color);
	int                      best       = 0;

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

			const t_cell        cand{ static_cast<int_fast16_t>(x), static_cast<int_fast16_t>(y) };
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

template<typename Traits>
int MasterAI<Traits>::evaluateLeafPosition(const SearchPosition<Traits>& position, t_cell cell)
{
	const Color sideToMove = position.sideToMove();
	const Color lastPlayed = (sideToMove == Color::Black) ? Color::White : Color::Black;

	const int createdPatterns =
		(lastPlayed == Color::Black) ? evaluateBlackPosition(position, cell) : evaluateWhitePosition(position, cell);

	if (createdPatterns >= MATE_THRESHOLD || createdPatterns <= -MATE_THRESHOLD)
		return createdPatterns;

	const bool sideToMoveNearCaptureWin = position.getCapturesForColor(sideToMove) >= 8;

	if (!sideToMoveNearCaptureWin && createdPatterns > -THREAT_SCAN_THRESHOLD &&
	    createdPatterns < THREAT_SCAN_THRESHOLD)
		return createdPatterns;

	const int sideToMoveBest = bestThreatNear(position, sideToMove, cell);
	return createdPatterns + signedFromAi(sideToMove, sideToMoveBest);
}

template<typename Traits> int MasterAI<Traits>::signedFromAi(Color side, int raw) const
{
	return (side == _aiColor) ? raw : -raw;
}

template<typename Traits> int MasterAI<Traits>::evaluatePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	int                   result       = 0;
	BitboardTool<Traits>& bitboardTool = BitboardTool<Traits>::instance();
	t_BWBoard<Traits>     board        = position.board();

	const Color side = (position.sideToMove() == Color::Black) ? Color::White : Color::Black;

	if (isWinAfterMove<Traits>(board, side, cell.x, cell.y))
		return signedFromAi(side, 1000000);

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

template<typename Traits>
int MasterAI<Traits>::evaluateBlackPosition(const SearchPosition<Traits>& position, t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	const auto& board = position.board();

	const int totalByBlack = position.getTotalwhiteCaptures();
	const int capsThisMove = position.getWhiteCaptures();
	const int captureScore = captureProgressScore(totalByBlack - capsThisMove, capsThisMove);

	if (totalByBlack >= CAPTURES_TO_WIN)
		return signedFromAi(Color::Black, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::Black, cell.x, cell.y))
	{
		const FiveVerdict verdict =
			judgeFiveAfterMove<Traits>(board, Color::Black, cell.x, cell.y, position.getCapturesForColor(Color::White));

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

template<typename Traits>
int MasterAI<Traits>::evaluateWhitePosition(const SearchPosition<Traits>& position, t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();

	const auto& board = position.board();

	const int totalByWhite = position.getTotalblackCaptures();
	const int capsThisMove = position.getBlackCaptures();
	const int captureScore = captureProgressScore(totalByWhite - capsThisMove, capsThisMove);

	if (totalByWhite >= CAPTURES_TO_WIN)
		return signedFromAi(Color::White, 1100000 + captureScore);

	if (isWinAfterMove<Traits>(board, Color::White, cell.x, cell.y))
	{
		const FiveVerdict verdict =
			judgeFiveAfterMove<Traits>(board, Color::White, cell.x, cell.y, position.getCapturesForColor(Color::Black));

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
