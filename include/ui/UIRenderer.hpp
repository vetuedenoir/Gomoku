#ifndef UIRENDERER_HPP
#define UIRENDERER_HPP

#include <SFML/Graphics.hpp>
#include <optional>
#include "game/contracts/contracts.hpp"
#include "game/board/GameBoard.hpp"

class Board;
class IGameController;
class MenuPage;

class UIRenderer
{
public:
	UIRenderer() = default;

	void renderMenu(sf::RenderWindow& w, MenuPage& page);

	void renderGame(sf::RenderWindow& w, Board& board, const IGameController& ctrl, CellStatus ghost,
	                std::optional<Move> suggestion = std::nullopt);

	void renderStats(sf::RenderWindow& w, const sf::Font& font, const Board& board, const IGameController& ctrl);

	void renderColorChoice(sf::RenderWindow& w, MenuPage& colorChoice);

	void renderWinScreen(sf::RenderWindow& w, MenuPage& winScreen);
};

#endif
