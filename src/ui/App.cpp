#include "ui/App.hpp"
#include "interface.hpp"
#include <algorithm>
#include <random>

#ifdef __APPLE__
    static const char *FONT = "/System/Library/Fonts/Helvetica.ttc";
#else
    static const char *FONT = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
#endif

// ── App constructor ───────────────────────────────────────────────────────────

App::App()
    : _window(sf::VideoMode(WIN_W, WIN_H), "Gomoku", sf::Style::Titlebar | sf::Style::Close)
{
    _window.setFramerateLimit(60);
    _font.loadFromFile(FONT);

    buildMainMenu();
    buildBoardSize();
    buildStoneColor();
    buildOpening();

    _states.push(AppState::MainMenu);
}

// ── App::run ──────────────────────────────────────────────────────────────────

void App::run()
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

// ── App::currentPage ──────────────────────────────────────────────────────────

MenuPage &App::currentPage()
{
    switch (_states.top())
    {
        case AppState::BoardSize:  return _boardSize;
        case AppState::StoneColor: return _stoneColor;
        case AppState::Opening:    return _opening;
        default:                   return _mainMenu;
    }
}

// ── App::navigateTo ───────────────────────────────────────────────────────────

void App::navigateTo(AppState s)
{
    _states.push(s);
    if (s == AppState::Game)
        startGame();
}

// ── App::goBack ───────────────────────────────────────────────────────────────

void App::goBack()
{
    if (_states.size() > 1)
        _states.pop();
}

// ── App::startGame ────────────────────────────────────────────────────────────

void App::startGame()
{
    float boardSize = std::min(WIN_W, WIN_H) * 0.90f;
    _board     = std::make_unique<Board>(WIN_W / 2.f, WIN_H / 2.f, boardSize);
    _gameBoard = std::make_unique<GameBoard>();
}

// ── App::handleEvent ──────────────────────────────────────────────────────────

void App::handleEvent(const sf::Event &event, sf::Vector2f mouse)
{
    if (event.type == sf::Event::Closed)
        _window.close();

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (_states.top() == AppState::Game)
        {
            int col = _board->getHoveredCol();
            int row = _board->getHoveredRow();
            if (col >= 0 && row >= 0)
                _gameBoard->placeStone(col, row);
        }
        else
        {
            currentPage().handleClick(mouse);
        }
    }
}

// ── App::update ───────────────────────────────────────────────────────────────

void App::update(sf::Vector2f mouse)
{
    if (_states.top() == AppState::Game)
        _board->updateHover(mouse);
    else
        currentPage().updateHover(mouse);
}

// ── App::render ───────────────────────────────────────────────────────────────

void App::render()
{
    _window.clear(BG);
    if (_states.top() == AppState::Game)
        _board->draw(_window, *_gameBoard);
    else
        currentPage().draw(_window);
    _window.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// Menu builders
// ─────────────────────────────────────────────────────────────────────────────

void App::buildMainMenu()
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
        if (auto *t = page.getText("title"))       win.draw(*t);
        if (auto *r = page.getRectangle("divider")) win.draw(*r);
        if (auto *t = page.getText("sub"))         win.draw(*t);
        if (auto *fi = page.getItem("play"))       fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))       fi->item.draw(win);
    });
}

void App::buildBoardSize()
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
        if (auto *t  = page.getText("title"))   win.draw(*t);
        if (auto *t  = page.getText("sub"))     win.draw(*t);
        if (auto *fi = page.getItem("15x15"))   fi->item.draw(win);
        if (auto *fi = page.getItem("19x19"))   fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))    fi->item.draw(win);
    });
}

void App::buildStoneColor()
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
        [this]() { _config.playerStone = 1; navigateTo(AppState::Opening); }
    ));
    _stoneColor.addItem("black", FonctionItem(
        Item("Black", _font, WIN_W / 2.f, 350.f),
        [this]() { _config.playerStone = 0; navigateTo(AppState::Opening); }
    ));
    _stoneColor.addItem("random", FonctionItem(
        Item("Random", _font, WIN_W / 2.f, 430.f),
        [this]() {
            static std::mt19937 rng(std::random_device{}());
            _config.playerStone = std::uniform_int_distribution<int>(0, 1)(rng);
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

void App::buildOpening()
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
        if (auto *t  = page.getText("title"))    win.draw(*t);
        if (auto *t  = page.getText("sub"))      win.draw(*t);
        if (auto *fi = page.getItem("normal"))   fi->item.draw(win);
        if (auto *fi = page.getItem("pro"))      fi->item.draw(win);
        if (auto *fi = page.getItem("longpro"))  fi->item.draw(win);
        if (auto *fi = page.getItem("swap"))     fi->item.draw(win);
        if (auto *fi = page.getItem("swap2"))    fi->item.draw(win);
        if (auto *fi = page.getItem("return"))   fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))     fi->item.draw(win);
    });
}
