#ifndef ACTIVEZONE_HPP
# define ACTIVEZONE_HPP

#include "bitboard/bitboard.hpp"

template<typename Traits>
class ActiveZone
{
    public:
        explicit ActiveZone(int radius);

        void initialize(const t_BWBoard<Traits>& board);

        const typename Traits::Bitboard& getCandidateMask() const noexcept;

        bool contains(int col, int row) const noexcept;

        int size() const noexcept;

        void clear();

        void setRadius(int radius) noexcept;

        int getRadius() const noexcept;

    private:
        typename Traits::Bitboard _candidateMask;
        int                       _radius;

        void addNeighborBits(int cx, int cy);
};

// ── Implementation ────────────────────────────────────────────────────────────

template<typename Traits>
ActiveZone<Traits>::ActiveZone(int radius) : _candidateMask({}), _radius(radius)
{}

template<typename Traits>
void ActiveZone<Traits>::initialize(const t_BWBoard<Traits>& board)
{
    _candidateMask = {};

    for (int y = 0; y < Traits::BOARD_SIZE; y++)
    {
        for (int x = 0; x < Traits::BOARD_SIZE; x++)
        {
            if (get_bb_generic<Traits>(board.black, x, y) ||
                get_bb_generic<Traits>(board.white, x, y))
                addNeighborBits(x, y);
        }
    }

    for (int i = 0; i < Traits::WORD_COUNT; i++)
        _candidateMask[i] &= ~(board.black[i] | board.white[i]);
}

template<typename Traits>
void ActiveZone<Traits>::addNeighborBits(int cx, int cy)
{
    for (int dy = -_radius; dy <= _radius; dy++)
    {
        for (int dx = -_radius; dx <= _radius; dx++)
        {
            int nx = cx + dx;
            int ny = cy + dy;
            if (in_board_generic<Traits>(nx, ny))
                set_bb_generic<Traits>(_candidateMask, nx, ny);
        }
    }
}

template<typename Traits>
const typename Traits::Bitboard& ActiveZone<Traits>::getCandidateMask() const noexcept
{
    return _candidateMask;
}

template<typename Traits>
bool ActiveZone<Traits>::contains(int col, int row) const noexcept
{
    return get_bb_generic<Traits>(_candidateMask, col, row);
}

template<typename Traits>
int ActiveZone<Traits>::size() const noexcept
{
    return popcount_bb_generic<Traits>(_candidateMask);
}

template<typename Traits>
void ActiveZone<Traits>::clear()
{
    _candidateMask = {};
}

template<typename Traits>
void ActiveZone<Traits>::setRadius(int radius) noexcept
{
    _radius = radius;
}

template<typename Traits>
int ActiveZone<Traits>::getRadius() const noexcept
{
    return _radius;
}

// ── Convenience aliases ───────────────────────────────────────────────────────

using ActiveZone19 = ActiveZone<BoardTraits<19>>;
using ActiveZone15 = ActiveZone<BoardTraits<15>>;

#endif // ACTIVEZONE_HPP
