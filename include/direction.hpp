#ifndef DIRECTION_HPP
#define DIRECTION_HPP

enum class Direction : int
{
	E  = 0,
	SE = 1,
	S  = 2,
	SW = 3,
	W  = 4,
	NW = 5,
	N  = 6,
	NE = 7
};

constexpr int DIR_DX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
constexpr int DIR_DY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

inline int dx(Direction d)
{
	return DIR_DX[static_cast<int>(d)];
}
inline int dy(Direction d)
{
	return DIR_DY[static_cast<int>(d)];
}

constexpr Direction LINE_DIRS[4] = { Direction::E, Direction::SE, Direction::S, Direction::NE };

#endif // DIRECTION_HPP
