#include "Gomoku.hpp"
#include "interface.hpp"
#include "logger/Logger.hpp"
#include "game/OpeningScript.hpp"
#include <algorithm>
#include <random>
#include <iostream>

#ifdef __APPLE__
    static const char *FONT = "/System/Library/Fonts/Helvetica.ttc";
#else
    static const char *FONT = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
#endif

Gomoku::Gomoku()
    : _window(sf::VideoMode(WIN_W, WIN_H), "Gomoku", sf::Style::Titlebar | sf::Style::Close)
{
    _window.setFramerateLimit(60);

    _font.loadFromFile(FONT);

    buildMainMenuPage();
    buildBoardSizePage();
    buildStoneColorPage();
    buildOpeningPage();

    _states.push(AppState::MainMenu);
}

void Gomoku::run()
{
    while (_window.isOpen())
    {
        sf::Vector2f mouse = _window.mapPixelToCoords(sf::Mouse::getPosition(_window));

        sf::Event event;

        while (_window.pollEvent(event))
            handleEvent(event, mouse);

        update(mouse);

        render();
    }
}


MenuPage &Gomoku::currentPage()
{
    switch (_states.top())
    {
        case AppState::BoardSize:  return _boardSize;
        case AppState::StoneColor: return _stoneColor;
        case AppState::Opening:    return _opening;
        default:                   return _mainMenu;
    }
}

void Gomoku::navigateTo(AppState s)
{
    _states.push(s);
    if (s == AppState::Game)
        startGame();
}

void Gomoku::goBack()
{
    if (_states.size() > 1)
        _states.pop();
}

void Gomoku::logConfig() const
{
    const char *stoneStr[]   = { "Black (plays first)", "White" };
    const char *openingStr[] = { "Normal", "Pro", "Long Pro", "Swap", "Swap 2" };

    Logger::info("CONFIG",
        std::string("board=")   + std::to_string(_config.boardSize) + "x" + std::to_string(_config.boardSize)
        + "  stone="   + stoneStr  [static_cast<int>(_config.playerStoneColor)]
        + "  opening=" + openingStr[static_cast<int>(_config.openingRule)]);
}

void Gomoku::startGame()
{
    logConfig();

    if (_config.boardSize == 19)
        _bitboardTool = std::make_unique<BitboardTool19>();
    else if (_config.boardSize == 15)
        _bitboardTool = std::make_unique<BitboardTool15>();
    else
        std::cerr << "Error: Impossible size: " << _config.boardSize << std::endl;

    float boardSize = std::min(WIN_W, WIN_H) * 0.90f;
    _board     = std::make_unique<Board>(WIN_W / 2.f, WIN_H / 2.f, boardSize, _config.boardSize);
    _gameState = std::make_unique<GameState>(_config.boardSize,
                                              _config.openingRule,
                                              _config.playerStoneColor);
}

// ── Ghost-colour computation ──────────────────────────────────────────────────

CellStatus Gomoku::computeGhostColor() const
{
    if (!_gameState)
        return CellStatus::Empty;

    switch (_gameState->phase)
    {
        case GamePhase::OpeningPlacement:
            return _gameState->nextOpeningColor();

        case GamePhase::NormalPlay:
            return (_gameState->board->currentSeat() == Seat::First)
                ? CellStatus::Black
                : CellStatus::White;

        case GamePhase::ColorChoice:
            return CellStatus::Empty;
    }
    return CellStatus::Empty;
}


void Gomoku::handleEvent(const sf::Event &event, sf::Vector2f mouse)
{
    if (event.type == sf::Event::Closed)
        _window.close();

    if (event.type != sf::Event::MouseButtonReleased
        || event.mouseButton.button != sf::Mouse::Left)
        return;

    if (_states.top() != AppState::Game)
    {
        currentPage().handleClick(mouse);
        return;
    }

    switch (_gameState->phase)
    {
        case GamePhase::OpeningPlacement:
        {
            int col = _board->getHoveredCol();
            int row = _board->getHoveredRow();
            if (col < 0 || row < 0)
                break;

            CellStatus forced = _gameState->nextOpeningColor();
            Move m{ col, row, forced };
            if (!applyOpeningMove(*_gameState, m))
                break;

            if (_gameState->phase == GamePhase::ColorChoice)
                buildColorChoicePage();
            break;
        }

        case GamePhase::ColorChoice:
            _colorChoice.handleClick(mouse);
            break;

        case GamePhase::NormalPlay:
        {
            int col = _board->getHoveredCol();
            int row = _board->getHoveredRow();
            if (col >= 0 && row >= 0)
            {
                const char* color = (_gameState->board->currentSeat() == Seat::First) ? "Black" : "White";
                if (_gameState->board->placeStone(col, row)) {
                    Logger::debug("NORMAL",
                        std::string(color) + " → (" + std::to_string(col) + "," + std::to_string(row) + ") ✓");
                        // test_bitboard(*_gameState->board, col, row);
                }
                else {
                    Logger::warn("NORMAL",
                        std::string("(") + std::to_string(col) + "," + std::to_string(row)
                        + ") rejected — cell occupied");                      
                }
            }
            break;
        }
    }
}

void Gomoku::update(sf::Vector2f mouse)
{
    if (_states.top() == AppState::Game)
    {
        _board->updateHover(mouse);
        if (_gameState->phase == GamePhase::ColorChoice)
            _colorChoice.updateHover(mouse);
    }
    else
    {
        currentPage().updateHover(mouse);
    }
}


void Gomoku::render()
{
    _window.clear(BG);

    if (_states.top() == AppState::Game)
        renderGame();
    else
        currentPage().draw(_window);

    _window.display();
}

void Gomoku::renderGame()
{
    _board->draw(_window, *_gameState->board, computeGhostColor());

    if (_gameState->phase == GamePhase::ColorChoice)
        renderColorChoicePage();
}


void Gomoku::buildColorChoicePage()
{
    _colorChoice.clear();

    const OpeningRule rule  = _gameState->openingRule;
    const Seat        actor = _gameState->currentActor;
    const float       horizontalCenterWindow    = WIN_W / 2.f;

    const bool threeOptions = (rule == OpeningRule::Swap2
                               && actor == Seat::Second
                               && _gameState->stepIdx == 1);

    const char* titleStr = threeOptions
        ? "Seat 2: Choose your option"
        : (actor == Seat::Second ? "Seat 2: Choose your colour"
                                 : "Seat 1: Choose your colour");

    sf::Text title = makeText(titleStr, _font, 22, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(horizontalCenterWindow, 378.f);

    sf::RectangleShape divider(sf::Vector2f(300.f, 1.f));
    divider.setFillColor(GOLD);
    divider.setOrigin(150.f, 0.5f);
    divider.setPosition(horizontalCenterWindow, 400.f);


    const char* label1 = (actor == Seat::Second) ? "Play White" : "Play Black";
    const char* label2 = (actor == Seat::Second) ? "Play Black" : "Play White";

    _colorChoice.addItem("opt1", FonctionItem(
        Item(label1,         _font, horizontalCenterWindow, 436.f),
        [this]() { _gameState->resolveColorChoice(false); }
    ));
    _colorChoice.addItem("opt2", FonctionItem(
        Item(label2,         _font, horizontalCenterWindow, 491.f),
        [this]() { _gameState->resolveColorChoice(true); }
    ));
    if (threeOptions)
    {
        _colorChoice.addItem("place2", FonctionItem(
            Item("Place 2 More", _font, horizontalCenterWindow, 546.f),
            [this]() { _gameState->continueOpeningPlacement(); }
        ));
    }

    _colorChoice.addText("title", title);
    _colorChoice.addRectangle("divider", divider);

    _colorChoice.setDrawFunction([](MenuPage& page, sf::RenderWindow& win) {
        if (auto *t  = page.getText("title"))        win.draw(*t);
        if (auto *r  = page.getRectangle("divider")) win.draw(*r);
        if (auto *fi = page.getItem("opt1"))         fi->item.draw(win);
        if (auto *fi = page.getItem("opt2"))         fi->item.draw(win);
        if (auto *fi = page.getItem("place2"))       fi->item.draw(win);
    });
}

void Gomoku::renderColorChoicePage()
{
    const bool threeOptions = _colorChoice.hasItem("place2");

    const float panelW  = 320.f;
    const float panelH  = threeOptions ? 243.f : 188.f;
    const float panelCY = threeOptions ? 465.f : 437.f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setOrigin(panelW / 2.f, panelH / 2.f);
    panel.setPosition(WIN_W / 2.f, panelCY);
    panel.setFillColor(sf::Color(10, 10, 20, 218));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(GOLD);
    _window.draw(panel);

    _colorChoice.draw(_window);
}

// ── Menu builders ─────────────────────────────────────────────────────────────

void Gomoku::buildMainMenuPage()
{
    sf::Text title = makeText("GOMOKU", _font, 80, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(WIN_W / 2.f, 150.f);
    _mainMenu.addText("title", title);

    sf::Text sub = makeText("Five in a row", _font, 20, DIM);
    sub.setPosition(WIN_W / 2.f, 218.f);
    _mainMenu.addText("sub", sub);

    sf::RectangleShape divider(sf::Vector2f(320.f, 2.f));
    divider.setFillColor(GOLD);
    divider.setOrigin(160.f, 1.f);
    divider.setPosition(WIN_W / 2.f, 240.f);
    _mainMenu.addRectangle("divider", divider);

    _mainMenu.addItem("play", FonctionItem(
        Item("Play", _font, WIN_W / 2.f, 340.f),
        [this]() { navigateTo(AppState::BoardSize); }
    ));
    _mainMenu.addItem("quit", FonctionItem(
        Item("Quit", _font, WIN_W / 2.f, 420.f),
        [this]() { _window.close(); }
    ));

    _mainMenu.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t = page.getText("title"))        win.draw(*t);
        if (auto *r = page.getRectangle("divider")) win.draw(*r);
        if (auto *t = page.getText("sub"))          win.draw(*t);
        if (auto *fi = page.getItem("play"))        fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))        fi->item.draw(win);
    });
}

void Gomoku::buildBoardSizePage()
{
    sf::Text title = makeText("GOMOKU", _font, 60, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(WIN_W / 2.f, 110.f);
    _boardSize.addText("title", title);

    sf::Text sub = makeText("Choose the size of the Goban", _font, 20, DIM);
    sub.setPosition(WIN_W / 2.f, 168.f);
    _boardSize.addText("sub", sub);

    _boardSize.addItem("15x15", FonctionItem(
        Item("15 x 15", _font, WIN_W / 2.f, 270.f),
        [this]() { _config.boardSize = 15; navigateTo(AppState::StoneColor); }
    ));
    _boardSize.addItem("19x19", FonctionItem(
        Item("19 x 19", _font, WIN_W / 2.f, 355.f),
        [this]() { _config.boardSize = 19; navigateTo(AppState::StoneColor); }
    ));
    _boardSize.addItem("quit", FonctionItem(
        Item("Quit", _font, WIN_W / 2.f, 450.f),
        [this]() { _window.close(); }
    ));

    _boardSize.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t  = page.getText("title"))  win.draw(*t);
        if (auto *t  = page.getText("sub"))    win.draw(*t);
        if (auto *fi = page.getItem("15x15"))  fi->item.draw(win);
        if (auto *fi = page.getItem("19x19"))  fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))   fi->item.draw(win);
    });
}

void Gomoku::buildStoneColorPage()
{
    sf::Text title = makeText("GOMOKU", _font, 60, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(WIN_W / 2.f, 90.f);
    _stoneColor.addText("title", title);

    sf::Text sub = makeText("Choose your Stone", _font, 20, DIM);
    sub.setPosition(WIN_W / 2.f, 148.f);
    _stoneColor.addText("sub", sub);

    sf::Text note = makeText("Black plays first", _font, 16, DIM);
    note.setPosition(WIN_W / 2.f, 176.f);
    _stoneColor.addText("note", note);

    _stoneColor.addItem("white", FonctionItem(
        Item("White", _font, WIN_W / 2.f, 270.f),
        [this]() { _config.playerStoneColor = StoneColor::White; navigateTo(AppState::Opening); }
    ));
    _stoneColor.addItem("black", FonctionItem(
        Item("Black", _font, WIN_W / 2.f, 350.f),
        [this]() { _config.playerStoneColor = StoneColor::Black; navigateTo(AppState::Opening); }
    ));
    _stoneColor.addItem("random", FonctionItem(
        Item("Random", _font, WIN_W / 2.f, 430.f),
        [this]() {
            static std::mt19937 rng(std::random_device{}());
            _config.playerStoneColor = static_cast<StoneColor>(std::uniform_int_distribution<int>(0, 1)(rng));
            navigateTo(AppState::Opening);
        }
    ));
    _stoneColor.addItem("return", FonctionItem(
        Item("Return", _font, WIN_W / 2.f - 140.f, 515.f),
        [this]() { goBack(); }
    ));
    _stoneColor.addItem("quit", FonctionItem(
        Item("Quit", _font, WIN_W / 2.f + 140.f, 515.f),
        [this]() { _window.close(); }
    ));

    _stoneColor.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t  = page.getText("title"))   win.draw(*t);
        if (auto *t  = page.getText("sub"))     win.draw(*t);
        if (auto *t  = page.getText("note"))    win.draw(*t);
        if (auto *fi = page.getItem("white"))   fi->item.draw(win);
        if (auto *fi = page.getItem("black"))   fi->item.draw(win);
        if (auto *fi = page.getItem("random"))  fi->item.draw(win);
        if (auto *fi = page.getItem("return"))  fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))    fi->item.draw(win);
    });
}

void Gomoku::buildOpeningPage()
{
    sf::Text title = makeText("GOMOKU", _font, 60, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(WIN_W / 2.f, 80.f);
    _opening.addText("title", title);

    sf::Text sub = makeText("Choose your Opening", _font, 20, DIM);
    sub.setPosition(WIN_W / 2.f, 132.f);
    _opening.addText("sub", sub);

    const float col1 = 250.f;
    const float col2 = 550.f;

    _opening.addItem("normal", FonctionItem(
        Item("Normal",   _font, col1, 220.f),
        [this]() { _config.openingRule = OpeningRule::Normal;  navigateTo(AppState::Game); }
    ));
    _opening.addItem("pro", FonctionItem(
        Item("Pro",      _font, col2, 220.f),
        [this]() { _config.openingRule = OpeningRule::Pro;     navigateTo(AppState::Game); }
    ));
    _opening.addItem("longpro", FonctionItem(
        Item("Long Pro", _font, col1, 300.f),
        [this]() { _config.openingRule = OpeningRule::LongPro; navigateTo(AppState::Game); }
    ));
    _opening.addItem("swap", FonctionItem(
        Item("Swap",     _font, col2, 300.f),
        [this]() { _config.openingRule = OpeningRule::Swap;    navigateTo(AppState::Game); }
    ));
    _opening.addItem("swap2", FonctionItem(
        Item("Swap 2",   _font, WIN_W / 2.f, 380.f),
        [this]() { _config.openingRule = OpeningRule::Swap2;   navigateTo(AppState::Game); }
    ));
    _opening.addItem("return", FonctionItem(
        Item("Return",   _font, col1, 490.f),
        [this]() { goBack(); }
    ));
    _opening.addItem("quit", FonctionItem(
        Item("Quit",     _font, col2, 490.f),
        [this]() { _window.close(); }
    ));

    _opening.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t  = page.getText("title"))   win.draw(*t);
        if (auto *t  = page.getText("sub"))     win.draw(*t);
        if (auto *fi = page.getItem("normal"))  fi->item.draw(win);
        if (auto *fi = page.getItem("pro"))     fi->item.draw(win);
        if (auto *fi = page.getItem("longpro")) fi->item.draw(win);
        if (auto *fi = page.getItem("swap"))    fi->item.draw(win);
        if (auto *fi = page.getItem("swap2"))   fi->item.draw(win);
        if (auto *fi = page.getItem("return"))  fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))    fi->item.draw(win);
    });
}
