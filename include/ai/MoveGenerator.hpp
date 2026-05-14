#ifndef MOVEGENERATOR_HPP
# define MOVEGENERATOR_HPP

#include "ai/ActiveZone.hpp"
#include "bitboard/bitboard.hpp"
#include "bitboard/pattern.hpp"

class MoveGenerator
{
    public:
        explicit MoveGenerator(int activeZoneRadius = 2);

        void generateLegalMoves(const t_BWBoard19& board, Color color, bitboard19& legalMoves) const;
        bool isLegalMove(const t_BWBoard19& board, int col, int row, Color color) const;

    private:
        int _activeZoneRadius;
};

#endif
