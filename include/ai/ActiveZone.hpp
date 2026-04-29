#ifndef ACTIVEZONE_HPP
# define ACTIVEZONE_HPP

#include "bitboard/bitboard.hpp"

class ActiveZone
{
    public:
        explicit ActiveZone(int radius);

        void initialize(const t_BWBoard19& board);

        const bitboard19& getCandidateMask() const noexcept;

        bool contains(int col, int row) const noexcept;

        int size() const noexcept;

        void clear();

        void setRadius(int radius) noexcept;

        int getRadius() const noexcept;

    private:
        bitboard19  _candidateMask;
        int         _radius;

        void addNeighborBits(int cx, int cy);
};

#endif
