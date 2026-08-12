#ifndef MASTER_AI_HPP
# define MASTER_AI_HPP

#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include "game/validation/MoveValidator.hpp"
#include "bitboard/bitboard.hpp"
#include "SearchPosition.hpp"
#include "optimization/TranspositionTable.hpp"
#include "logger/Logger.hpp"
#include "config/config.hpp"
#include <optional>
#include <variant>
#include <limits>
#include <vector>
#include <chrono>
#include <algorithm>

using t_BWBoard_variant = std::variant<t_BWBoard19, t_BWBoard15>;

struct SearchStats
{
    int    nodesVisited   = 0;  // total minimax() calls
    int    nodesEvaluated = 0;  // leaf nodes reaching evaluatePosition()
    int    nodesPruned    = 0;  // alpha-beta cut-offs
    int    maxDepthSeen   = 0;  // deepest ply actually reached
    int    ttHits         = 0;  // [minimax] TT entries reused with sufficient depth
    int    ttCutoffs      = 0;  // [minimax] TT bound that produced an immediate cutoff
    int    ttStores       = 0;  // [minimax] entries written to the TT
    int    ttOrderingHits = 0;  // [minimax] TT bestMove hoisted to front for ordering
    int    ttRootHits         = 0;  // [root] probe found a matching entry
    int    ttRootOrderingHits = 0;  // [root] entry bestMove located & moved to front
    int    ttRootExactSeeds   = 0;  // [root] EXACT entry (depth>=maxDepth) seeded best move/score
    int    forcedNodes        = 0;  // nœuds où la liste a été réduite aux parades d'un cinq
    int    bestScore      = 0;
    t_cell bestMove       = {-1, -1};
};

template <typename T> struct MasterAITestAccess;

struct MoveSorted
{
	t_cell			    move;
	const TTEntry*		hit;
	MoveStateHash		stateHash;
};


template<typename Traits>
class MasterAI
{
	template <typename T> friend struct MasterAITestAccess;

	public:
		explicit MasterAI(int depth = DEPTH, int activeZoneRadius = ACTIVE_ZONE_RADIUS, Color aiColor = Color::Black);
		
		t_cell findBestMove(
			const SearchPosition<Traits>& position,
			Color color
		);
		
		void setSearchDepth(int depth) noexcept;
		int  getSearchDepth() const noexcept;

		int  getStonesCapturedByAI() const noexcept { return _stoneCapturedByAI; }
		void setStonesCapturedByAI(int count) noexcept { _stoneCapturedByAI = count; }

		int  getStonesCapturedByOPP() const noexcept { return _stoneCapturedByOPP;}
		void setStonesCapturedByOPP(int count) noexcept { _stoneCapturedByOPP = count; }

		Color getAIColor() const noexcept { return _aiColor; }
		void setAIColor(Color color) noexcept { _aiColor = color; }

		const SearchStats& lastSearchStats() const noexcept { return _stats; }

	private:
		int                     _maxDepth;
		int					 	_stoneCapturedByAI = 0;
		int					 	_stoneCapturedByOPP = 0;
		Color                   _aiColor      = Color::Black;
		MoveGenerator<Traits>   _moveGenerator;
		TranspositionTable      _tt;
		
		using Clock     = std::chrono::steady_clock;
		using TimePoint = std::chrono::time_point<Clock>;

		SearchStats             _stats;

		// --- Ordonnancement dynamique : killer moves + history heuristic ---------
		// Profondeur (ply) maximale gérée par les tables killer. Au-delà, on ignore
		// simplement les heuristiques (aucun impact sur la correction).
		static constexpr int    MAX_SEARCH_PLY = 64;
		// Deux "killer moves" par ply : coups (souvent silencieux) ayant provoqué
		// une coupure beta chez un frère au même ply. Réinitialisés à chaque
		// recherche racine.
		t_cell                  _killers[MAX_SEARCH_PLY][2];
		// History heuristic indexée [camp][case] : accumule depth^2 à chaque coupure
		// pour départager les coups silencieux de score statique égal.
		int                     _history[2][Traits::CELL_COUNT];

		void resetOrderingHeuristics();
		bool isKillerMove(int ply, t_cell move) const;
		int  historyScore(Color mover, t_cell move) const;
		void recordCutoff(int ply, t_cell move, int depth, Color mover);

		int signedFromAi(Color side, int raw) const;

		int minimax(SearchPosition<Traits>& position, t_cell cell, int depth, int alpha, int beta);
		
		int evaluatePosition(const SearchPosition<Traits>& position, t_cell cell);

		int evaluateBlackPosition(const SearchPosition<Traits>& position, t_cell cell);
		int evaluateWhitePosition(const SearchPosition<Traits>& position, t_cell cell);

		// Éval de feuille bilatérale : ce que le dernier coup a créé (signé IA)
		// + meilleure menace / réponse du camp au trait dans la zone active.
		int evaluateLeafPosition(const SearchPosition<Traits>& position, t_cell cell);

		// Meilleure offense légale pour `color` près de `anchor` (Chebyshev),
		// ∩ zone active. Pas de défense (déjà dans createdSigned).
		int bestThreatNear(const SearchPosition<Traits>& position, Color color, t_cell anchor);

		EvaluatedMove rawShapeScore(const t_BWBoard<Traits>& board, t_cell cell, Color color);

		// Clé full de référence : light + cross. Le tri ne l'appelle plus
		// (rawShapeScoreLight puis upgradeLightToFull donnent le même résultat).
		EvaluatedMove rawShapeScoreV2(const t_BWBoard<Traits>& board, t_cell cell, Color color, int capturesBefore);

		// Clé light : caps + five/four/broken + open_three (légalité). Pas de cross.
		EvaluatedMove rawShapeScoreLight(const t_BWBoard<Traits>& board, t_cell cell, Color color, int capturesBefore);

		// Complète une clé light avec le seul maillon manquant (check_cross),
		// et seulement si son `stage` laisse cette possibilité ouverte.
		void upgradeLightToFull(EvaluatedMove& move, const t_BWBoard<Traits>& board,
			Color color, int capturesBefore);

		// Coups forcés : si l'adversaire aligne cinq au coup suivant, réduit la
		// liste aux réponses qui peuvent encore changer l'issue. Renvoie true si
		// une restriction a eu lieu.
		bool restrictToForcedReplies(const t_BWBoard<Traits>& board,
			MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits>>& ordered, Color mover);

		// Enrichit la clé d'ordonnancement : score = offense + défense/2.
		// `offense` garde captures/légalité du camp au trait (pour makeMove) ;
		// la défense = ce que l'adversaire créerait sur la même case (valeur de blocage).
		void addDefenseToOrderingScore(EvaluatedMove& offense,
			const t_BWBoard<Traits>& board, Color side);
};

using MasterAI19 = MasterAI<BoardTraits<19>>;
using MasterAI15 = MasterAI<BoardTraits<15>>;

#include "ai/MasterAI.inl"
#include "ai/heuristique.inl"

#endif
