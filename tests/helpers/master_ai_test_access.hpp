#pragma once

#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "optimization/TranspositionTable.hpp"

// White-box bridge into MasterAI for unit tests. Declared as a friend in
// MasterAI, so it may reach the private minimax() entry point (allowing an
// explicit [alpha, beta] window and isMaximizing flag) and the private
// transposition table (to assert stored bounds/flags).
template <typename Traits>
struct MasterAITestAccess
{
	static int search(MasterAI<Traits>& ai, SearchPosition<Traits>& position, t_cell cell, int depth, int alpha, int beta)
	{
		return ai.minimax(position, cell, depth, alpha, beta);
	}

	// Même entrée, avec l'état de sursis hérité du parent (règle de la capture
	// finale) : `pending` appartient toujours au camp au trait de ce nœud.
	static int searchWithPending(MasterAI<Traits>& ai, SearchPosition<Traits>& position, t_cell cell,
		int depth, int alpha, int beta, std::optional<PendingWin> pending)
	{
		return ai.minimax(position, cell, depth, alpha, beta, pending);
	}

	static const TranspositionTable& tt(const MasterAI<Traits>& ai)
	{
		return ai._tt;
	}

	static TranspositionTable& ttMutable(MasterAI<Traits>& ai)
	{
		return ai._tt;
	}

	static EvaluatedMove lightKey(MasterAI<Traits>& ai, const t_BWBoard<Traits>& board, t_cell cell, Color color, int capturesBefore)
	{
		return ai.computeLightScore(board, cell, color, capturesBefore);
	}

	static EvaluatedMove fullKey(MasterAI<Traits>& ai, const t_BWBoard<Traits>& board, t_cell cell, Color color, int capturesBefore)
	{
		return ai.rawShapeScoreV2(board, cell, color, capturesBefore);
	}

	static void upgrade(MasterAI<Traits>& ai, EvaluatedMove& move, const t_BWBoard<Traits>& board, Color color, int capturesBefore)
	{
		ai.upgradeLightToFull(move, board, color, capturesBefore);
	}
};
