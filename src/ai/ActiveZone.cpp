#include "ai/ActiveZone.hpp"
#include <cstring>

ActiveZone::ActiveZone(int radius) : _radius(radius)
{
    memset(_candidateMask, 0, sizeof(_candidateMask));
}

void ActiveZone::initialize(const t_BWBoard19& board)
{
    memset(_candidateMask, 0, sizeof(_candidateMask));

    for (int y = 0; y < 19; y++)
    {
        for (int x = 0; x < 19; x++)
        {
            int pos = index(x, y);
            if (get(board.black, pos) || get(board.white, pos))
                addNeighborBits(x, y);
        }
    }

    for (int i = 0; i < 6; i++)
        _candidateMask[i] &= ~(board.black[i] | board.white[i]);
}

void ActiveZone::addNeighborBits(int cx, int cy)
{
    for (int dy = -_radius; dy <= _radius; dy++)
    {
        for (int dx = -_radius; dx <= _radius; dx++)
        {
            int nx = cx + dx;
            int ny = cy + dy;
            if (in_board(nx, ny))
                set(_candidateMask, nx, ny);
        }
    }
}

const bitboard19& ActiveZone::getCandidateMask() const noexcept
{
    return _candidateMask;
}

bool ActiveZone::contains(int col, int row) const noexcept
{
    return get(_candidateMask, index(col, row));
}

int ActiveZone::size() const noexcept
{
    return popcount_bb(_candidateMask);
}

void ActiveZone::clear()
{
    memset(_candidateMask, 0, sizeof(_candidateMask));
}

void ActiveZone::setRadius(int radius) noexcept
{
    _radius = radius;
}

int ActiveZone::getRadius() const noexcept
{
    return _radius;
}
