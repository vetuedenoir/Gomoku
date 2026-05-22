#include "bitboard/pattern.hpp"
#include "logger/Logger.hpp"

static bool is_double_three_score(int score)
{
	if (score <= 0)
		return false;
	if (score == SCORE_3_FULL || score == SCORE_3_HOLE
		|| score >= SCORE_FULL_EXTERN)
		return false;
	if (score < SCORE_3_FULL)
		return true;
	if (score >= SCORE_DOUBLE_FULL_FULL_EXTERN
		&& score <= SCORE_DOUBLE_HOLE_HOLE_INTERN2)
		return true;
	return false;
}

template<typename Traits>
bool is_double_three_move(const t_BWBoard<Traits>& board, int col, int row, const Color color)
{
	static t_PatternList_Groupe3 table[Traits::CELL_COUNT][4] = {};
	static bool initialized = false;
	if (!initialized)
	{
		build_lookup_table3<Traits>(table);
		initialized = true;
	}

	t_BWBoard<Traits> temp = board;
	typename Traits::Bitboard& moverPlane = (color == Color::Black) ? temp.black : temp.white;
	typename Traits::Bitboard& oppPlane   = (color == Color::Black) ? temp.white : temp.black;
	set_bb_generic<Traits>(moverPlane, col, row);

	const int score = check_three_align<Traits>(table, moverPlane, oppPlane, col, row);
	Logger::debug("DOUBLE-3", " score=" + std::to_string(score));

	return is_double_three_score(score);
}

template bool is_double_three_move<BoardTraits<19>>(const t_BWBoard<BoardTraits<19>>&, int, int, Color);
template bool is_double_three_move<BoardTraits<15>>(const t_BWBoard<BoardTraits<15>>&, int, int, Color);
