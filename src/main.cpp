#include "interface.hpp"
#include "ui/Board.hpp"
#include "game/GameBoard.hpp"
#include <algorithm>

int main()
{
	sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "Gomoku",
							sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(60);

	const float boardSize = std::min(WIN_W, WIN_H) * 0.90f;
	Board     board(WIN_W / 2.f, WIN_H / 2.f, boardSize);
	GameBoard gameBoard;

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
				int col = board.getHoveredCol();
				int row = board.getHoveredRow();
				if (col >= 0 && row >= 0)
					gameBoard.placeStone(col, row);
			}
		}

		board.updateHover(mouse);

		window.clear(BG);
		board.draw(window, gameBoard);
		window.display();
	}

	return 0;
}
