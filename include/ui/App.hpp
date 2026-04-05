#ifndef APP_HPP
# define APP_HPP

#include <SFML/Graphics.hpp>
#include <stack>
#include <memory>

#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "game/GameBoard.hpp"
#include "game/GameConfig.hpp"

enum class AppState { MainMenu, BoardSize, StoneColor, Opening, Game };

class App
{
public:
    App();
    void run();

private:
    sf::RenderWindow     _window;
    sf::Font             _font;

    std::stack<AppState> _states;
    GameConfig           _config;

    MenuPage _mainMenu;
    MenuPage _boardSize;
    MenuPage _stoneColor;
    MenuPage _opening;

    std::unique_ptr<Board>     _board;
    std::unique_ptr<GameBoard> _gameBoard;

    // Navigation
    MenuPage &currentPage();
    void      navigateTo(AppState s);
    void      goBack();
    void      startGame();

    // Menu builders — called once in the constructor
    void buildMainMenu();
    void buildBoardSize();
    void buildStoneColor();
    void buildOpening();

    // Main loop helpers
    void handleEvent(const sf::Event &event, sf::Vector2f mouse);
    void update(sf::Vector2f mouse);
    void render();
};

#endif
