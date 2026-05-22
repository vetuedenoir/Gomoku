#include "ui/Board.hpp"

static const sf::Color LINE_COLOR   ( 60,  35,  10);
static const sf::Color STONE_BLACK  ( 30,  30,  30);
static const sf::Color STONE_WHITE  (230, 230, 230);
static const sf::Color GHOST_BLACK  ( 30,  30,  30, 140);
static const sf::Color GHOST_WHITE  (230, 230, 230, 140);

Board::Board(float x, float y, float size, int gridN)
    : _gridN(gridN), _hoveredCol(-1), _hoveredRow(-1)
{
    _bounds   = sf::FloatRect(x - size / 2.f, y - size / 2.f, size, size);
    _cellSize = size / (_gridN - 1);
    _stoneR   = _cellSize * 0.42f;

    _hasBg = _bgTexture.loadFromFile("assets/gomoku_board_background.png");
    if (_hasBg)
    {
        sf::Vector2u ts = _bgTexture.getSize();
        _bgSprite.setTexture(_bgTexture);
        _bgSprite.setPosition(_bounds.left, _bounds.top);
        _bgSprite.setScale(
            _bounds.width  / static_cast<float>(ts.x),
            _bounds.height / static_cast<float>(ts.y)
        );
    }

    for (int i = 0; i < _gridN; ++i)
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

    _hoveredCol = std::max(0, std::min(_gridN - 1, _hoveredCol));
    _hoveredRow = std::max(0, std::min(_gridN - 1, _hoveredRow));
}


void Board::draw(sf::RenderWindow &window, const GameBoard &gameBoard,
                 CellStatus ghostHint) const
{
    if (_hasBg)
    {
        window.draw(_bgSprite);
    }
    
    for (const sf::RectangleShape &h : _hLines) window.draw(h);
    for (const sf::RectangleShape &v : _vLines) window.draw(v);

    // Ghost stone — shown only when hovering a free cell and we know the next colour.
    if (_hoveredCol >= 0 && _hoveredRow >= 0
        && gameBoard.isFree(_hoveredCol, _hoveredRow)
        && ghostHint != CellStatus::Empty)
    {
        sf::Color ghostColor = (ghostHint == CellStatus::Black) ? GHOST_BLACK : GHOST_WHITE;

        float ghostX = _bounds.left + _hoveredCol * _cellSize;
        float ghostY = _bounds.top  + _hoveredRow * _cellSize;

        sf::CircleShape ghost(_stoneR);
        ghost.setOrigin(_stoneR, _stoneR);
        ghost.setPosition(ghostX, ghostY);
        ghost.setFillColor(ghostColor);
        window.draw(ghost);
    }

    // Placed stones — read directly from GameBoard state
    for (int row = 0; row < _gridN; ++row)
    {
        for (int col = 0; col < _gridN; ++col)
        {
            CellStatus cell = gameBoard.getCell(col, row);
            if (cell == CellStatus::Empty) continue;

            sf::CircleShape stone(_stoneR);
            stone.setOrigin(_stoneR, _stoneR);
            stone.setPosition(_bounds.left + col * _cellSize,
                              _bounds.top  + row * _cellSize);
            stone.setFillColor(cell == CellStatus::Black ? STONE_BLACK : STONE_WHITE);
            window.draw(stone);
        }
    }
}
