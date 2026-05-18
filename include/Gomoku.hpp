#ifndef GOMOKU_HPP
# define GOMOKU_HPP

#include <SFML/Graphics.hpp>
#include <stack>
#include <memory>

#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "ui/UIRenderer.hpp"
#include "game/GameController.hpp"


enum class AppState { MainMenu, BoardSize, StoneColor, Opening, Game, GameOver };

class Gomoku
{
    private:
        sf::RenderWindow     _window;
        sf::Font             _font;
        UIRenderer           _renderer;

        std::stack<AppState> _states;
        GameConfig           _config;

        MenuPage _mainMenu;
        MenuPage _boardSize;
        MenuPage _stoneColor;
        MenuPage _opening;
        MenuPage _colorChoice;
        MenuPage _winScreen;

        std::unique_ptr<Board>          _board;
        std::unique_ptr<GameController> _controller;

        MenuPage &currentPage();
        void      navigateTo(AppState s);
        void      update(sf::Vector2f mouse);
        void      goBack();
        void      startGame();

        void buildMainMenuPage();
        void buildBoardSizePage();
        void buildStoneColorPage();
        void buildOpeningPage();
        void buildColorChoicePage();
        void buildWinScreenPage(Color winner, int capturesBlack, int capturesWhite);

        void onBoardSizeSelected(int size);
        void onStoneColorSelected(StoneColor color);
        void onOpeningRuleSelected(OpeningRule rule);

        void logConfig() const;
        void resetToMainMenu();

        CellStatus computeGhostColor() const;

        void handleEvent(const sf::Event &event, sf::Vector2f mouse);
        void render();

    public:
        Gomoku();
        void run();
};

#endif
