#include "../include/interface.hpp"
// ── main ──────────────────────────────────────────────────────────────────────

# ifdef __APPLE__
	static const char        *FONT   = "/System/Library/Fonts/Helvetica.ttc";
# else
	static const char        *FONT   = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
# endif


int main()
{
	sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "Gomoku",
							sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(60);

	sf::Font font;
	if (!font.loadFromFile(FONT))
		return 1;

	MenuePage first;
	// Title
	sf::Text title = makeText("GOMOKU", font, 80, GOLD);
	title.setStyle(sf::Text::Bold);
	title.setPosition(WIN_W / 2.f, 160.f);

	// Divider line under title
	sf::RectangleShape divider(sf::Vector2f(320.f, 2.f));
	divider.setFillColor(GOLD);
	divider.setOrigin(160.f, 1.f);
	divider.setPosition(WIN_W / 2.f, 215.f);

	// Subtitle
	sf::Text sub = makeText("Five in a row", font, 20, DIM);
	sub.setPosition(WIN_W / 2.f, 240.f);

	// Menu items
	std::vector<Item> items;
	items.push_back(Item("Play",     font, WIN_W / 2.f, 360.f));
	items.push_back(Item("Quit",     font, WIN_W / 2.f, 440.f));

	while (window.isOpen())
	{
		sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::MouseButtonReleased &&
				event.mouseButton.button == sf::Mouse::Left)
			{
				if (items[0].contains(mouse))   // Play
				{ /* TODO: switch to game state */ }
				else if (items[1].contains(mouse)) // Quit
					window.close();
			}
		}

		for (std::size_t i = 0; i < items.size(); ++i)
			items[i].setHovered(items[i].contains(mouse));

		window.clear(BG);
		drawGrid(window);
		window.draw(divider);
		window.draw(title);
		window.draw(sub);
		for (std::size_t i = 0; i < items.size(); ++i)
			items[i].draw(window);
		window.display();
	}

	return 0;
}
