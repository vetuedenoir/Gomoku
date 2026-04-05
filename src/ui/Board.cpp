#include "ui/Board.hpp"
#include <cmath>

static const sf::Color LINE_COLOR   (230, 230, 230);
static const sf::Color STONE_BLACK  ( 30,  30,  30);
static const sf::Color STONE_WHITE  (230, 230, 230);
// Ghost: current player's color at half opacity
static const sf::Color GHOST_BLACK  ( 30,  30,  30, 140);
static const sf::Color GHOST_WHITE  (230, 230, 230, 140);

Board::Board(float x, float y, float size)
    : _hoveredCol(-1), _hoveredRow(-1)
{
    _bounds   = sf::FloatRect(x - size / 2.f, y - size / 2.f, size, size);
    _cellSize = size / (GRID_N - 1);
    _stoneR   = _cellSize * 0.42f;

    for (int i = 0; i < GRID_N; ++i)
    {
        float offset = i * _cellSize;

        sf::RectangleShape h(sf::Vector2f(size, 1.f));
        h.setOrigin(size / 2.f, 0.5f);
        h.setPosition(x, _bounds.top + offset);
        h.setFillColor(LINE_COLOR);
        _hLines.push_back(h);

        sf::RectangleShape v(sf::Vector2f(1.f, size));
        v.setOrigin(0.5f, size / 2.f);
        v.setPosition(_bounds.left + offset, y);
        v.setFillColor(LINE_COLOR);
        _vLines.push_back(v);
    }
}

bool Board::contains(sf::Vector2f pt) const
{
    return _bounds.contains(pt);
}

void Board::updateHover(sf::Vector2f pt)
{
    if (!_bounds.contains(pt))
    {
        _hoveredCol = -1;
        _hoveredRow = -1;
        return;
    }

    _hoveredCol = static_cast<int>(std::round((pt.x - _bounds.left) / _cellSize));
    _hoveredRow = static_cast<int>(std::round((pt.y - _bounds.top)  / _cellSize));

    _hoveredCol = std::max(0, std::min(GRID_N - 1, _hoveredCol));
    _hoveredRow = std::max(0, std::min(GRID_N - 1, _hoveredRow));
}


void Board::draw(sf::RenderWindow &window, const GameBoard &gameBoard) const
{
    for (const sf::RectangleShape &h : _hLines) window.draw(h);
    for (const sf::RectangleShape &v : _vLines) window.draw(v);

    // Ghost stone — only when hovering a free cell
    if (_hoveredCol >= 0 && _hoveredRow >= 0 && gameBoard.isFree(_hoveredCol, _hoveredRow))
    {
        sf::Color ghostColor = (gameBoard.getCurrentPlayer() == 0) ? GHOST_BLACK : GHOST_WHITE;

        float ghostX = _bounds.left + _hoveredCol * _cellSize;
        float ghostY = _bounds.top  + _hoveredRow * _cellSize;

        sf::CircleShape ghost(_stoneR);
        ghost.setOrigin(_stoneR, _stoneR);
        ghost.setPosition(ghostX, ghostY);
        ghost.setFillColor(ghostColor);
        window.draw(ghost);
    }

    // Placed stones — read directly from GameBoard state
    for (int row = 0; row < GRID_N; ++row)
    {
        for (int col = 0; col < GRID_N; ++col)
        {
            int cell = gameBoard.getCell(col, row);
            if (cell == 0) continue;

            sf::CircleShape stone(_stoneR);
            stone.setOrigin(_stoneR, _stoneR);
            stone.setPosition(_bounds.left + col * _cellSize,
                              _bounds.top  + row * _cellSize);
            stone.setFillColor(cell == 1 ? STONE_BLACK : STONE_WHITE);
            window.draw(stone);
        }
    }
}
