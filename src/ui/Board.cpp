#include "ui/Board.hpp"

static const sf::Color LINE_COLOR(60, 35, 10);
static const sf::Color STONE_BLACK(30, 30, 30);
static const sf::Color STONE_WHITE(230, 230, 230);
static const sf::Color GHOST_BLACK(30, 30, 30, 140);
static const sf::Color GHOST_WHITE(230, 230, 230, 140);

Board::Board(float x, float y, float size, int gridN)
    : _gridN(gridN), _bgSprite(_bgTexture), _hoveredCol(-1), _hoveredRow(-1)
{
	_bounds   = sf::FloatRect(sf::Vector2f(x - size / 2.f, y - size / 2.f), sf::Vector2f(size, size));
	_cellSize = size / (_gridN - 1);
	_stoneR   = _cellSize * 0.42f;

	_hasBg = _bgTexture.loadFromFile("assets/gomoku_board_background.png");
	if (_hasBg)
	{
		sf::Vector2u ts = _bgTexture.getSize();
		_bgSprite.setTexture(_bgTexture, true);
		_bgSprite.setPosition(sf::Vector2f(sfc::left(_bounds), sfc::top(_bounds)));
		_bgSprite.setScale(sf::Vector2f(sfc::width(_bounds) / static_cast<float>(ts.x), sfc::height(_bounds) / static_cast<float>(ts.y)));
	}

	for (int i = 0; i < _gridN; ++i)
	{
		float offset = i * _cellSize;

		sf::RectangleShape h(sf::Vector2f(size, 1.f));
		h.setOrigin(sf::Vector2f(size / 2.f, 0.5f));
		h.setPosition(sf::Vector2f(x, sfc::top(_bounds) + offset));
		h.setFillColor(LINE_COLOR);
		_hLines.push_back(h);

		sf::RectangleShape v(sf::Vector2f(1.f, size));
		v.setOrigin(sf::Vector2f(0.5f, size / 2.f));
		v.setPosition(sf::Vector2f(sfc::left(_bounds) + offset, y));
		v.setFillColor(LINE_COLOR);
		_vLines.push_back(v);
	}
}

bool Board::contains(sf::Vector2f pt) const
{
	return _bounds.contains(pt);
}

void Board::drawHighlight(sf::RenderWindow& window, int col, int row) const
{
	if (col < 0 || row < 0 || col >= _gridN || row >= _gridN)
		return;

	const float x = sfc::left(_bounds) + col * _cellSize;
	const float y = sfc::top(_bounds) + row * _cellSize;
	const float r = _stoneR * 1.05f;

	static const sf::Color HINT(220, 175, 80, 235);

	sf::CircleShape ring(r);
	ring.setOrigin(sf::Vector2f(r, r));
	ring.setPosition(sf::Vector2f(x, y));
	ring.setFillColor(sf::Color::Transparent);
	ring.setOutlineThickness(3.f);
	ring.setOutlineColor(HINT);
	window.draw(ring);

	const float     dotR = _stoneR * 0.22f;
	sf::CircleShape dot(dotR);
	dot.setOrigin(sf::Vector2f(dotR, dotR));
	dot.setPosition(sf::Vector2f(x, y));
	dot.setFillColor(HINT);
	window.draw(dot);
}

void Board::updateHover(sf::Vector2f pt)
{
	if (!_bounds.contains(pt))
	{
		_hoveredCol = -1;
		_hoveredRow = -1;
		return;
	}

	_hoveredCol = static_cast<int>(std::round((pt.x - sfc::left(_bounds)) / _cellSize));
	_hoveredRow = static_cast<int>(std::round((pt.y - sfc::top(_bounds)) / _cellSize));

	_hoveredCol = std::max(0, std::min(_gridN - 1, _hoveredCol));
	_hoveredRow = std::max(0, std::min(_gridN - 1, _hoveredRow));
}

void Board::draw(sf::RenderWindow& window, const GameBoard& gameBoard, CellStatus ghostHint) const
{
	if (_hasBg)
	{
		window.draw(_bgSprite);
	}

	for (const sf::RectangleShape& h : _hLines)
		window.draw(h);
	for (const sf::RectangleShape& v : _vLines)
		window.draw(v);

	if (_hoveredCol >= 0 && _hoveredRow >= 0 && gameBoard.isFree(_hoveredCol, _hoveredRow) &&
	    ghostHint != CellStatus::Empty)
	{
		sf::Color ghostColor = (ghostHint == CellStatus::Black) ? GHOST_BLACK : GHOST_WHITE;

		float ghostX = sfc::left(_bounds) + _hoveredCol * _cellSize;
		float ghostY = sfc::top(_bounds) + _hoveredRow * _cellSize;

		sf::CircleShape ghost(_stoneR);
		ghost.setOrigin(sf::Vector2f(_stoneR, _stoneR));
		ghost.setPosition(sf::Vector2f(ghostX, ghostY));
		ghost.setFillColor(ghostColor);
		window.draw(ghost);
	}

	for (int row = 0; row < _gridN; ++row)
	{
		for (int col = 0; col < _gridN; ++col)
		{
			CellStatus cell = gameBoard.getCell(col, row);
			if (cell == CellStatus::Empty)
				continue;

			sf::CircleShape stone(_stoneR);
			stone.setOrigin(sf::Vector2f(_stoneR, _stoneR));
			stone.setPosition(sf::Vector2f(sfc::left(_bounds) + col * _cellSize, sfc::top(_bounds) + row * _cellSize));
			stone.setFillColor(cell == CellStatus::Black ? STONE_BLACK : STONE_WHITE);
			window.draw(stone);
		}
	}
}
