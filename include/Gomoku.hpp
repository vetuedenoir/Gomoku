#ifndef GOMOKU_HPP
# define GOMOKU_HPP

#include <SFML/Graphics.hpp>
#include <stack>
#include <memory>
#include <optional>
#include <string>

#include "game/contracts/contracts.hpp"
#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "ui/UIRenderer.hpp"
#include "ui/StatusBanner.hpp"
#include "game/controller/IGameController.hpp"
#include "game/demo/RuleDemos.hpp"


enum class AppState { MainMenu, RuleDemos, BoardSize, StoneColor, Opening, Game, GameOver };

class Gomoku
{
    private:
        sf::RenderWindow     _window;
        sf::Font             _font;
        UIRenderer           _renderer;

        std::stack<AppState> _states;
        GameConfig           _config;

        MenuPage _mainMenu;
        MenuPage _ruleDemos;
        MenuPage _boardSize;
        MenuPage _stoneColor;
        MenuPage _opening;
        MenuPage _colorChoice;
        MenuPage _winScreen;

        std::unique_ptr<Board>          _board;
        std::unique_ptr<IGameController> _controller;
        
        std::optional<Move> _suggestion;
        std::optional<Move> _demoKeyCell;
        std::string         _demoHint;
        sf::Clock           _winRevealClock;
        bool                _awaitingWinScreen = false;
        StatusBanner        _statusBanner;

        MenuPage &currentPage();
        void      navigateTo(AppState s);
        void      update(sf::Vector2f mouse);
        void      goBack();
        void      startGame();

        void buildMainMenuPage();
        void buildRuleDemosPage();
        void buildBoardSizePage();
        void buildStoneColorPage();
        void buildOpeningPage();
        void buildColorChoicePage();
        void buildWinScreenPage(const Color winner, int capturesBlack, int capturesWhite);

        void clearRuleDemo();
        void startRuleDemo(const RuleDemo& demo);

        void onBoardSizeSelected(int size);
        void onStoneColorSelected(const Color color);
        void onOpeningProtocolSelected(OpeningProtocol openingProtocol);

        void logConfig() const;
        void resetToMainMenu();

        CellStatus computeGhostColor() const;
        bool       isAITurn() const;
        bool       isGameOver();
        void       announceAiOpeningDecision();
        void       refreshStatusBanner();

        void requestSuggestion();
        void clearSuggestion();

        void handleEvent(const sf::Event &event, sf::Vector2f mouse);
        void render();

    public:
        Gomoku();
        void run();
};

#endif
