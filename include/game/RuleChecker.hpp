#ifndef RULECHECKER_HPP
# define RULECHECKER_HPP

#include "bitboard/bitboard.hpp"
#include "game/contracts/Color.hpp"

class RuleChecker
{
public:
    bool isLegal(const t_BWBoard19& board, int col, int row, Color color) const;
};

#endif
