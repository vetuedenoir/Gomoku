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

	static const TranspositionTable& tt(const MasterAI<Traits>& ai)
	{
		return ai._tt;
	}

	static TranspositionTable& ttMutable(MasterAI<Traits>& ai)
	{
		return ai._tt;
	}
};
