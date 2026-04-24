#include "ai/ActiveZone.hpp"

ActiveZone::ActiveZone(int radius) : _radius(radius) {}

// generate candidates from all stones (one-time cost)
void ActiveZone::initialize(const GameBoard& board)
{
    _candidates.clear();

    const int size = board.getSize();

    for (int x = 0; x < size; ++x)
    {
        for (int y = 0; y < size; ++y)
        {
            if (board.getCell(x, y) != CellStatus::Empty)
            {
                addNeighbors({x, y, CellStatus::Neighbor}, board);
            }
        }
    }
}

void ActiveZone::addNeighbors(const Move& move, const GameBoard& board)
{
    for (int dx = -_radius; dx <= _radius; ++dx)
    {
        for (int dy = -_radius; dy <= _radius; ++dy)
        {
            int nx = move.col + dx;
            int ny = move.row + dy;

            if (!board.isInside(nx, ny))
                continue;

            if (!board.isFree(nx, ny))
                continue;

            _candidates.insert({nx, ny, CellStatus::Neighbor});
        }
    }
}

const ActiveZone::CandidateSet& ActiveZone::getCandidates() const noexcept
{
    return _candidates;
}

bool ActiveZone::contains(const Move& m) const noexcept
{
    return _candidates.find(m) != _candidates.end();
}

std::size_t ActiveZone::size() const noexcept
{
    return _candidates.size();
}

void ActiveZone::clear()
{
    _candidates.clear();
}

void    ActiveZone::setRadius(int radius) noexcept
{
    _radius = radius;
}
        
int     ActiveZone::getRadius() const noexcept
{
    return _radius;
}