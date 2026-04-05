#ifndef BOARD_HPP
# define BOARD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include "game/GameBoard.hpp"

static const int GRID_N = 19;

class Board
{
public:
    Board(float x, float y, float size);

    bool contains(sf::Vector2f pt) const;
    void updateHover(sf::Vector2f pt);

    int getHoveredCol() const { return _hoveredCol; }
    int getHoveredRow() const { return _hoveredRow; }

    void draw(sf::RenderWindow &window, const GameBoard &gameBoard) const;

private:
    sf::FloatRect                   _bounds;
    float                           _cellSize;
    float                           _stoneR;
    std::vector<sf::RectangleShape> _hLines;
    std::vector<sf::RectangleShape> _vLines;

    int _hoveredCol;
    int _hoveredRow;
};

#endif
