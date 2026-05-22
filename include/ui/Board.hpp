#ifndef BOARD_HPP
# define BOARD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include "game/board/GameBoard.hpp"
#include <iostream>

static const int GRID_MAX = 19;

class Board
{
    private:
        sf::FloatRect                   _bounds;
        float                           _cellSize;
        float                           _stoneR;
        int                             _gridN;
        std::vector<sf::RectangleShape> _hLines;
        std::vector<sf::RectangleShape> _vLines;

        sf::Texture _bgTexture;
        sf::Sprite  _bgSprite;
        bool        _hasBg = false;

        int _hoveredCol;
        int _hoveredRow;

    public:
        Board(float x, float y, float size, int gridN);

        bool contains(sf::Vector2f pt) const;
        void updateHover(sf::Vector2f pt);

        int getHoveredCol() const { return _hoveredCol; }
        int getHoveredRow() const { return _hoveredRow; }

        void draw(sf::RenderWindow &window, const GameBoard &gameBoard, CellStatus ghostHint) const;
};

#endif
