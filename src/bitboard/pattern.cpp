#include "bitboard/pattern.hpp"
#include "logger/Logger.hpp"

// ── is_double_three_move ─────────────────────────────────────────────────────
//
// Checks whether placing `color` at (col, row) on `board` creates a
// forbidden double-three. The lookup table is lazily built per Traits
// specialization (template function statics are per-instantiation in C++).

template<typename Traits>
bool is_double_three_move(const t_BWBoard<Traits>& board, int col, int row, Color color)
{
    static t_PatternList_Groupe3 table[361][4] = {};
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

    int score = check_three_align<Traits>(table, moverPlane, oppPlane, col, row);
    Logger::debug("DOUBLE-3", " score=" + std::to_string(score));

    int free_threes = 0;
    return free_threes >= 2;
}

template bool is_double_three_move<BoardTraits<19>>(const t_BWBoard<BoardTraits<19>>&, int, int, Color);
template bool is_double_three_move<BoardTraits<15>>(const t_BWBoard<BoardTraits<15>>&, int, int, Color);
