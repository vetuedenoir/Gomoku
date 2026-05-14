#ifndef GOMOKU_HPP
# define GOMOKU_HPP

#include <SFML/Graphics.hpp>
#include <stack>
#include <memory>
#include <iostream>

#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "game/GameBoard.hpp"
#include "game/contracts/GameConfig.hpp"
#include "game/GameState.hpp"

#include "bitboard/bitboard.hpp"
#include "bitboard/pattern.hpp"
#include "bitboard/BitboardTool.hpp"
#include "bitboard/BitboardTool19.hpp"
#include "bitboard/BitboardTool15.hpp"


enum class AppState { MainMenu, BoardSize, StoneColor, Opening, Game };

class Gomoku
{
    private:
        sf::RenderWindow     _window;
        sf::Font             _font;

        std::stack<AppState> _states;
        GameConfig           _config;


        MenuPage _mainMenu;
        MenuPage _boardSize;
        MenuPage _stoneColor;
        MenuPage _opening;
        MenuPage _colorChoice;

        std::unique_ptr<Board>      _board;
        std::unique_ptr<GameState>  _gameState;
        std::unique_ptr<IBitboardTool> _bitboardTool;

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

        void logConfig() const;

        CellStatus computeGhostColor() const;

        void handleEvent(const sf::Event &event, sf::Vector2f mouse);
    
        void renderGame();
        void renderColorChoicePage();
        void render();

    public:
        Gomoku();
        void run();
};

#endif
