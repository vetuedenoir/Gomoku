#include "ai/MasterAI.hpp"
#include "game/turn/WinDetector.hpp"
#include "logger/Logger.hpp"

// Score d'un alignement gagnant (mat). Les scores dont la magnitude dépasse
// MATE_THRESHOLD sont des scores de mat : ils encodent une distance (ply) et
// doivent être ré-ajustés à l'entrée/sortie de la table de transposition.
static constexpr int WIN_SCORE      = 1000000;
static constexpr int MATE_THRESHOLD = 900000;

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
t_cell	MasterAI<Traits>::findBestMove(
	const SearchPosition<Traits>& position, Color color)
{
	_aiColor = color;

	_stats        = SearchStats{};
	_timeExceeded = false;
	if (_timeLimitMs)
		_searchStart = Clock::now();

	const std::vector<t_cell> rootMoves = _moveGenerator.generateMoves(position.board(), position.sideToMove());

	LOG_DEBUG("AI", "[findBestMove] depth=" + std::to_string(_maxDepth)
	              + "  root candidates=" + std::to_string(rootMoves.size()));

	if (rootMoves.empty())
		return {-1, -1};

	int bestScore = std::numeric_limits<int>::min();
	t_cell bestMove = {-1, -1};

	for (const t_cell& move : rootMoves)
	{
		SearchPosition<Traits> newPosition = position;
		
		newPosition.makeMove(move.x, move.y, colorToCell(position.sideToMove()));

		int score = minimax(newPosition, move, _maxDepth - 1,
		                    std::numeric_limits<int>::min(),
		                    std::numeric_limits<int>::max(), false);

		// if (_timeExceeded)
		// {
		// 	LOG_DEBUG("AI", "[findBestMove] time limit reached — returning best so far");
		// 	break;
		// }

		const std::string marker = (score > bestScore) ? " ← best" : "";
		const std::string macro  = scoreMacroLabel(score);
		LOG_DEBUG("AI", "  root (" + std::to_string(move.x) + ","
		              + std::to_string(move.y) + ")  score =  " + std::to_string(score) + macro + marker);

		if (score > bestScore)
		{
			bestScore = score;
			bestMove  = move;
		}
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
		+ "  maxDepth="  + std::to_string(_stats.maxDepthSeen));
	LOG_SUPPRESS(_stats.nodesVisited, _stats.nodesEvaluated, _stats.nodesPruned, _stats.maxDepthSeen, pruningPct);
	LOG_DEBUG("AI", "[findBestMove] tt stores=" + std::to_string(_stats.ttStores));
	LOG_DEBUG("AI", "[findBestMove] tt hits=" + std::to_string(_stats.ttHits));
	LOG_DEBUG("AI", "[findBestMove] tt cutoffs=" + std::to_string(_stats.ttCutoffs));
	

	return bestMove;
}

template <typename Traits>
int	MasterAI<Traits>::minimax(SearchPosition<Traits>& position, t_cell cell,
			int depth, int alpha, int beta, bool isMaximizing)
{
	++_stats.nodesVisited;

	const int currentDepth = _maxDepth - depth;
	if (currentDepth > _stats.maxDepthSeen)
		_stats.maxDepthSeen = currentDepth;

	// TODO: who does the flip ?
	// The side that just played is the opponent of sideToMove() (makeMove flipped it).
	const Color lastPlayed = (position.sideToMove() == Color::Black) ? Color::White : Color::Black;

	// ne verifie pas les captures gagnantes, seulement les alignements de 5
	if (isWinAfterMove<Traits>(position.board(), lastPlayed, cell.x, cell.y))
	{
		++_stats.nodesEvaluated;
		// Score relatif à la racine : un mat plus proche (currentDepth petit)
		// vaut plus, pour préférer la victoire la plus rapide / retarder la défaite.
		const int mate = WIN_SCORE - currentDepth;
		return (lastPlayed == _aiColor) ? mate : -mate;
	}

	if (depth == 0)
	{
		++_stats.nodesEvaluated;
		if (lastPlayed == Color::Black)
			return evaluateBlackPosition(position, cell);
		return evaluateWhitePosition(position, cell);
		// return evaluatePosition(position, cell);
	}

	const std::vector<t_cell> moves = _moveGenerator.generateMoves(position.board(), position.sideToMove());
	

	if (moves.empty())
	{
		++_stats.nodesEvaluated;
		if (lastPlayed == Color::Black)
			return evaluateBlackPosition(position, cell);
		return evaluateWhitePosition(position, cell);
		// return evaluatePosition(position, cell);
	}

	const uint64_t ttKey = position.zobristHash();

	if (const TTEntry* ttHit = _tt.probe(ttKey); ttHit && ttHit->depth >= depth)
	{
		++_stats.ttHits;
		const int ttScore = ttScoreFromEntry(ttHit->score, currentDepth);
		if (ttHit->flag == TTFlag::Exact)
			return ttScore;
		if (ttHit->flag == TTFlag::LowerBound)
			alpha = std::max(alpha, ttScore);
		else if (ttHit->flag == TTFlag::UpperBound)
			beta = std::min(beta, ttScore);

		if (alpha >= beta)
			return ttScore;
	}

	int alphaSearch = alpha;
	int betaSearch = beta;
	t_cell bestMove = {-1, -1};

	int bestEval = 0;

	if (isMaximizing)
	{ 
		bestEval = std::numeric_limits<int>::min();

		for (const t_cell& move : moves)
		{
			const CellStatus stone = colorToCell(position.sideToMove());
			//ne gere pas les captures.
			position.makeMove(move.x, move.y, stone);
			
			int eval = minimax(position, move, depth - 1, alpha, beta, false);
			
			position.undoMove(move.x, move.y, stone);

			if (eval > bestEval)
			{
				bestEval = eval;
				bestMove = move;
			}

			alpha = std::max(alpha, eval);
			
			if (beta <= alpha)
			{
				++_stats.nodesPruned;
				break;
			}
		}

		// return maxEval;
	}
	else
	{
		bestEval = std::numeric_limits<int>::max();

		for (const t_cell& move : moves)
		{
			const CellStatus stone = colorToCell(position.sideToMove());
			position.makeMove(move.x, move.y, stone);
			
			int eval = minimax(position, move, depth - 1, alpha, beta, true);

			position.undoMove(move.x, move.y, stone);

			if (eval < bestEval)
			{
				bestEval = eval;
				bestMove = move;
			}
			beta = std::min(beta, eval);
			if (beta <= alpha)
			{
				++_stats.nodesPruned;
				break;
			}
		}
		// return minEval;
	}


	// Résultat tronqué par le timeout : ne pas polluer la TT.
	if (_timeExceeded)
		return bestEval;

	// When storing a LowerBound or UpperBound, we are storing knowledge about a cutoff that already happened.
	TTFlag flag;

	if (bestEval <= alphaSearch)        // Fail-low : on n'a pas dépassé alpha
		flag = TTFlag::UpperBound;
	else if (bestEval >= betaSearch)    // Fail-high : coupure
		flag = TTFlag::LowerBound;
	else
		flag = TTFlag::Exact;            // alphaSearch < bestEval < betaSearch
	
	// TODO: cell or move ?!!! 
	_tt.store(position.zobristHash(), ttScoreToEntry(bestEval, currentDepth), depth, flag,
	          {bestMove.x, bestMove.y, CellStatus::Empty});
	++_stats.ttStores;

	return bestEval;
}

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
	auto board = position.board();

	if (isWinAfterMove<Traits>(board, Color::Black, cell.x, cell.y))
		return signedFromAi(Color::Black, 1000000);

	int result = tool.check_open_four(board.black, board.white, cell.x, cell.y);
	if (result == 2)
		return signedFromAi(Color::Black, 500000);
	if (result == 1)
		return signedFromAi(Color::Black, 5000);

	if (tool.check_super_four(board.black, board.white, cell.x, cell.y))
		return signedFromAi(Color::Black, 60000);

	if (tool.check_broken_four(board.black, board.white, cell.x, cell.y))
		return signedFromAi(Color::Black, 6000);

	result = tool.check_cross(board.black, board.white, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::Black, cross_score(result));

	result = tool.check_open_three(board.black, board.white, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::Black, score_open_three(result));

	return 0;
}


template <typename Traits>
int MasterAI<Traits>::evaluateWhitePosition(
	const SearchPosition<Traits>& position,
	t_cell cell)
{
	BitboardTool<Traits>& tool = BitboardTool<Traits>::instance();
	auto board = position.board();

	if (isWinAfterMove<Traits>(board, Color::White, cell.x, cell.y))
		return signedFromAi(Color::White, 1000000);

	int result = tool.check_open_four(board.black, board.white, cell.x, cell.y);
	if (result == 2)
		return signedFromAi(Color::White, 500000);
	if (result == 1)
		return signedFromAi(Color::White, 5000);

	if (tool.check_super_four(board.white, board.black, cell.x, cell.y))
		return signedFromAi(Color::White, 60000);

	if (tool.check_broken_four(board.white, board.black, cell.x, cell.y))
		return signedFromAi(Color::White, 6000);

	result = tool.check_cross(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::White, cross_score(result));

	result = tool.check_open_three(board.white, board.black, cell.x, cell.y);
	if (result)
		return signedFromAi(Color::White, score_open_three(result));

	return 0;
}