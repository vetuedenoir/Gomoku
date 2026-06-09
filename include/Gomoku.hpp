#ifndef GOMOKU_HPP
# define GOMOKU_HPP

#include <SFML/Graphics.hpp>
#include <stack>
#include <memory>

#include "game/contracts/contracts.hpp"
#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "ui/UIRenderer.hpp"
#include "game/controller/IGameController.hpp"


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
        std::unique_ptr<IGameController> _controller;

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
        void buildWinScreenPage(const Color winner, int capturesBlack, int capturesWhite);

        void onBoardSizeSelected(int size);
        void onStoneColorSelected(const Color color);
        void onOpeningProtocolSelected(OpeningProtocol openingProtocol);

        void logConfig() const;
        void resetToMainMenu();

        CellStatus computeGhostColor() const;
        bool       isAITurn() const;

        void handleEvent(const sf::Event &event, sf::Vector2f mouse);
        void render();

    public:
        Gomoku();
        void run();
};

#endif
