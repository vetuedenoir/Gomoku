#include "Gomoku.hpp"
#include "interface.hpp"
#include "logger/Logger.hpp"
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <thread>


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
    const char *openingStr[] = { "Standard", "Pro", "Long Pro", "Swap", "Swap 2" };

    LOG_INFO("CONFIG",
        std::string("board=")   + std::to_string(_config.boardSize) + "x" + std::to_string(_config.boardSize)
        + "  stone="   + stoneStr  [static_cast<int>(_config.playerColor)]
        + "  opening=" + openingStr[static_cast<int>(_config.openingProtocol)]);
    LOG_SUPPRESS(stoneStr[static_cast<int>(_config.playerColor)], openingStr[static_cast<int>(_config.openingProtocol)]);
}

void Gomoku::onBoardSizeSelected(int size)
{
    _config.boardSize = size;
    navigateTo(AppState::StoneColor);
}

void Gomoku::onStoneColorSelected(const Color color)
{
    _config.playerColor = color;
    navigateTo(AppState::Opening);
}

void Gomoku::onOpeningProtocolSelected(OpeningProtocol openingProtocol)
{
    _config.openingProtocol = openingProtocol;
    navigateTo(AppState::Game);
}

void Gomoku::startGame()
{
    logConfig();

    float boardSize = std::min(WIN_W, WIN_H) * 0.90f;
    _board      = std::make_unique<Board>(WIN_W / 2.f, WIN_H / 2.f, boardSize, _config.boardSize);
    _controller = makeGameController(_config);

    LOG_INFO("GAME DEBUG", "player actor: " + seatStr(_controller->playerActor().seat) + " " + (_controller->playerActor().color == Color::Black ? "Black" : "White"));
    LOG_INFO("GAME DEBUG", "ai actor: " + seatStr(_controller->aiActor().seat) + " " + (_controller->aiActor().color == Color::Black ? "Black" : "White"));
    LOG_INFO("GAME DEBUG", "current actor: " + seatStr(_controller->currentActor().seat) + " " + (_controller->currentActor().color == Color::Black ? "Black" : "White"));
}

// ── Ghost-colour computation ──────────────────────────────────────────────────

CellStatus Gomoku::computeGhostColor() const
{
    if (!_controller)
        return CellStatus::Empty;

    switch (_controller->phase())
    {
        case GamePhase::Opening:
            return _controller->nextOpeningColor();

        case GamePhase::Standard:
            return (_controller->currentColor() == Color::Black)
                    ? CellStatus::Black
                    : CellStatus::White;

        case GamePhase::ColorChoice:
            return CellStatus::Empty;
    }
    return CellStatus::Empty;
}


bool Gomoku::isAITurn() const
{
    if (!_controller) return false;
    return _controller->currentActor().seat == _controller->aiActor().seat;
}

void Gomoku::handleEvent(const sf::Event &event, sf::Vector2f mouse)
{
    if (event.type == sf::Event::Closed)
        _window.close();

    if (event.type != sf::Event::MouseButtonReleased
        || event.mouseButton.button != sf::Mouse::Left)
        return;

    if (_states.top() == AppState::GameOver)
    {
        _winScreen.handleClick(mouse);
        return;
    }

    if (_states.top() != AppState::Game)
    {
        currentPage().handleClick(mouse);
        return;
    }

    switch (_controller->phase())
    {
        case GamePhase::Opening:
        {
            int col = _board->getHoveredCol();
            int row = _board->getHoveredRow();
            if (col < 0 || row < 0)
                break;

            if (!_controller->handleOpeningClick(col, row))
                break;

            if (_controller->phase() == GamePhase::ColorChoice)
            {
                LOG_INFO("ACTOR DEBUG", "buildColorChoicePage on opening click");
                buildColorChoicePage();
            }
            break;
        }

        case GamePhase::ColorChoice:
            LOG_INFO("COLORCHOICE", "handleEvent");
            _colorChoice.handleClick(mouse);
            break;

        case GamePhase::Standard:
        {
            int col = _board->getHoveredCol();
            int row = _board->getHoveredRow();
            if (col < 0 || row < 0)
                break;

            auto result = _controller->submitMove(col, row);
            
            if (result == MoveResult::Win)
            {
                buildWinScreenPage(_controller->getColorFromWinningActor().value(),
                                   _controller->captureCount(Color::Black),
                                   _controller->captureCount(Color::White));
                navigateTo(AppState::GameOver);
            }


            break;
        }
    }
}

void Gomoku::update(sf::Vector2f mouse)
{
    if (_states.top() == AppState::GameOver)
    {
        _winScreen.updateHover(mouse);
    }
    else if (_states.top() == AppState::Game)
    {
        _board->updateHover(mouse);
    }
    else
    {
        currentPage().updateHover(mouse);
    }

    if (_config.aiOpponent && _states.top() == AppState::Game && isAITurn())
    {
        LOG_INFO("AI DEBUG", "AI turn");
       _controller->requestAIMove();
    }
}


void Gomoku::render()
{
    _window.clear(BG);

    if (_states.top() == AppState::GameOver)
    {
        _renderer.renderGame(_window, *_board, *_controller, CellStatus::Empty);
        _renderer.renderWinScreen(_window, _winScreen);
    }
    else if (_states.top() == AppState::Game)
    {
        _renderer.renderGame(_window, *_board, *_controller, computeGhostColor());
        if (_controller->phase() == GamePhase::ColorChoice)
            _renderer.renderColorChoice(_window, _colorChoice);
    }
    else
    {
        _renderer.renderMenu(_window, currentPage());
    }

    _window.display();
}

void Gomoku::resetToMainMenu()
{
    while (!_states.empty())
        _states.pop();
    _states.push(AppState::MainMenu);
    _board.reset();
    _controller.reset();
}

void Gomoku::buildColorChoicePage()
{
    _colorChoice.clear();

    const Actor       actor = _controller->currentActor();

    LOG_INFO("COLORCHOICE", "actor: " + seatStr(actor.seat) + " " + (actor.color == Color::Black ? "Black" : "White"));
    LOG_SUPPRESS(actor.seat, actor.color);

    const Seat        seat = _controller->currentActor().seat;

    const OpeningProtocol openingProtocol  = _controller->openingProtocol();
    const bool threeOptions = (openingProtocol == OpeningProtocol::Swap2
                               && seat == Seat::Second
                               && _controller->stepIdx() == 1);

    

    const char* titleStr = threeOptions
        ? "Seat 2: Choose your option"
        : (seat == Seat::Second ? "Seat 2: Choose your colour"
                                 : "Seat 1: Choose your colour");

    sf::Text title = makeText(titleStr, _font, FONT_CC, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.4725f);

    const float divW = WIN_W * 0.30f;
    sf::RectangleShape divider(sf::Vector2f(divW, 1.f));
    divider.setFillColor(GOLD);
    divider.setOrigin(divW / 2.f, 0.5f);
    divider.setPosition(CX, WIN_H * 0.50f);

    const char* label1 = (seat == Seat::Second) ? "Play White" : "Play Black";
    const char* label2 = (seat == Seat::Second) ? "Play Black" : "Play White";

    _colorChoice.addItem("opt1", FonctionItem(
        Item(label1,         _font, CX, WIN_H * 0.545f),
        [this]() { _controller->resolveColorChoice(false); }
    ));
    _colorChoice.addItem("opt2", FonctionItem(
        Item(label2,         _font, CX, WIN_H * 0.614f),
        [this]() { _controller->resolveColorChoice(true); }
    ));
    if (threeOptions)
    {
        _colorChoice.addItem("place2", FonctionItem(
            Item("Place 2 More", _font, CX, WIN_H * 0.6825f),
            [this]() { _controller->continueOpeningPlacement(); }
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

void Gomoku::buildWinScreenPage(const Color winner, int capturesBlack, int capturesWhite)
{
    _winScreen.clear();

    const char* winnerStr = (winner == Color::Black) ? "Black Wins!" : "White Wins!";

    sf::Text title = makeText(winnerStr, _font, FONT_ML, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.3375f);
    _winScreen.addText("title", title);

    std::string scoreStr =
        "Black: " + std::to_string(capturesBlack / 2) + " cap  |  "
        "White: " + std::to_string(capturesWhite / 2) + " cap";
    sf::Text score = makeText(scoreStr, _font, FONT_SM, DIM);
    score.setPosition(CX, WIN_H * 0.431f);
    _winScreen.addText("score", score);

    _winScreen.addItem("again", FonctionItem(
        Item("Play Again", _font, CX, WIN_H * 0.5375f),
        [this]() { resetToMainMenu(); }
    ));
    _winScreen.addItem("quit", FonctionItem(
        Item("Quit", _font, CX, WIN_H * 0.6375f),
        [this]() { _window.close(); }
    ));

    _winScreen.setDrawFunction([](MenuPage& page, sf::RenderWindow& win) {
        if (auto *t  = page.getText("title"))  win.draw(*t);
        if (auto *t  = page.getText("score"))  win.draw(*t);
        if (auto *fi = page.getItem("again"))  fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))   fi->item.draw(win);
    });
}

// ── Menu builders ─────────────────────────────────────────────────────────────

void Gomoku::buildMainMenuPage()
{
    sf::Text title = makeText("GOMOKU", _font, FONT_XL, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.1875f);
    _mainMenu.addText("title", title);

    sf::Text sub = makeText("Five in a row", _font, FONT_MD, DIM);
    sub.setPosition(CX, WIN_H * 0.2725f);
    _mainMenu.addText("sub", sub);

    const float divW = WIN_W * 0.32f;
    sf::RectangleShape divider(sf::Vector2f(divW, 2.f));
    divider.setFillColor(GOLD);
    divider.setOrigin(divW / 2.f, 1.f);
    divider.setPosition(CX, WIN_H * 0.30f);
    _mainMenu.addRectangle("divider", divider);

    _mainMenu.addItem("play", FonctionItem(
        Item("Play", _font, CX, WIN_H * 0.425f),
        [this]() { navigateTo(AppState::BoardSize); }
    ));
    _mainMenu.addItem("quit", FonctionItem(
        Item("Quit", _font, CX, WIN_H * 0.525f),
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
    sf::Text title = makeText("GOMOKU", _font, FONT_LG, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.1375f);
    _boardSize.addText("title", title);

    sf::Text sub = makeText("Choose the size of the Goban", _font, FONT_MD, DIM);
    sub.setPosition(CX, WIN_H * 0.21f);
    _boardSize.addText("sub", sub);

    _boardSize.addItem("15x15", FonctionItem(
        Item("15 x 15", _font, CX, WIN_H * 0.3375f),
        [this]() { onBoardSizeSelected(15); }
    ));
    _boardSize.addItem("19x19", FonctionItem(
        Item("19 x 19", _font, CX, WIN_H * 0.444f),
        [this]() { onBoardSizeSelected(19); }
    ));
    _boardSize.addItem("quit", FonctionItem(
        Item("Quit", _font, CX, WIN_H * 0.5625f),
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
    sf::Text title = makeText("GOMOKU", _font, FONT_LG, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.1125f);
    _stoneColor.addText("title", title);

    sf::Text sub = makeText("Choose your Stone", _font, FONT_MD, DIM);
    sub.setPosition(CX, WIN_H * 0.185f);
    _stoneColor.addText("sub", sub);

    sf::Text note = makeText("Black plays first", _font, FONT_XS, DIM);
    note.setPosition(CX, WIN_H * 0.22f);
    _stoneColor.addText("note", note);

    _stoneColor.addItem("white", FonctionItem(
        Item("White", _font, CX, WIN_H * 0.3375f),
        [this]() { onStoneColorSelected(Color::White); }
    ));
    _stoneColor.addItem("black", FonctionItem(
        Item("Black", _font, CX, WIN_H * 0.4375f),
        [this]() { onStoneColorSelected(Color::Black); }
    ));
    _stoneColor.addItem("random", FonctionItem(
        Item("Random", _font, CX, WIN_H * 0.5375f),
        [this]() {
            static std::mt19937 rng(std::random_device{}());
            onStoneColorSelected(static_cast<Color>(
            std::uniform_int_distribution<int>(0, 1)(rng)));
        }
    ));
    _stoneColor.addItem("return", FonctionItem(
        Item("Return", _font, CX - WIN_W * 0.14f, WIN_H * 0.644f),
        [this]() { goBack(); }
    ));
    _stoneColor.addItem("quit", FonctionItem(
        Item("Quit", _font, CX + WIN_W * 0.14f, WIN_H * 0.644f),
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
    sf::Text title = makeText("GOMOKU", _font, FONT_LG, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.10f);
    _opening.addText("title", title);

    sf::Text sub = makeText("Choose your Opening", _font, FONT_MD, DIM);
    sub.setPosition(CX, WIN_H * 0.165f);
    _opening.addText("sub", sub);

    const float col1 = WIN_W * 0.25f;
    const float col2 = WIN_W * 0.55f;

    _opening.addItem("Standard", FonctionItem(
        Item("Standard",   _font, col1, WIN_H * 0.275f),
        [this]() { onOpeningProtocolSelected(OpeningProtocol::Standard); }
    ));
    _opening.addItem("pro", FonctionItem(
        Item("Pro",      _font, col2, WIN_H * 0.275f),
        [this]() { onOpeningProtocolSelected(OpeningProtocol::Pro); }
    ));
    _opening.addItem("longpro", FonctionItem(
        Item("Long Pro", _font, col1, WIN_H * 0.375f),
        [this]() { onOpeningProtocolSelected(OpeningProtocol::LongPro); }
    ));
    _opening.addItem("swap", FonctionItem(
        Item("Swap",     _font, col2, WIN_H * 0.375f),
        [this]() { onOpeningProtocolSelected(OpeningProtocol::Swap); }
    ));
    _opening.addItem("swap2", FonctionItem(
        Item("Swap 2",   _font, CX, WIN_H * 0.475f),
        [this]() { onOpeningProtocolSelected(OpeningProtocol::Swap2); }
    ));
    _opening.addItem("return", FonctionItem(
        Item("Return",   _font, col1, WIN_H * 0.6125f),
        [this]() { goBack(); }
    ));
    _opening.addItem("quit", FonctionItem(
        Item("Quit",     _font, col2, WIN_H * 0.6125f),
        [this]() { _window.close(); }
    ));

    _opening.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t  = page.getText("title"))   win.draw(*t);
        if (auto *t  = page.getText("sub"))     win.draw(*t);
        if (auto *fi = page.getItem("Standard"))  fi->item.draw(win);
        if (auto *fi = page.getItem("pro"))     fi->item.draw(win);
        if (auto *fi = page.getItem("longpro")) fi->item.draw(win);
        if (auto *fi = page.getItem("swap"))    fi->item.draw(win);
        if (auto *fi = page.getItem("swap2"))   fi->item.draw(win);
        if (auto *fi = page.getItem("return"))  fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))    fi->item.draw(win);
    });
}
