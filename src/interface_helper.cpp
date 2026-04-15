#include "interface.hpp"


void centerOrigin(sf::Text &t)
{
	sf::FloatRect b = t.getLocalBounds();
	t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
}

sf::Text makeText(const std::string &str, const sf::Font &font,
						  unsigned int size, sf::Color color)
{
	sf::Text t(str, font, size);
	t.setFillColor(color);
	centerOrigin(t);
	return t;
}

void drawGrid(sf::RenderWindow &window)
{
	const int	lines = 12;
	const float  step = 48.f;
	const float  ox = (WIN_W - step * (lines - 1)) / 2.f;
	const float  oy = (WIN_H - step * (lines - 1)) / 2.f;
	sf::Color    c(255, 255, 255, 12);

	sf::RectangleShape line;
	line.setFillColor(c);

	// vertical
	line.setSize(sf::Vector2f(1.f, step * (lines - 1)));
	for (int i = 0; i < lines; ++i)
	{
		line.setPosition(ox + i * step, oy);
		window.draw(line);
	}
	// horizontal
	line.setSize(sf::Vector2f(step * (lines - 1), 1.f));
	for (int i = 0; i < lines; ++i)
	{
		line.setPosition(ox, oy + i * step);
		window.draw(line);
	}
}
