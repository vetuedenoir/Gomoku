#include "Gomoku.hpp"
#include "interface.hpp"
#include "logger/Logger.hpp"
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>


#ifdef __APPLE__
    static const char *FONT = "/System/Library/Fonts/Helvetica.ttc";
#else
    static const char *FONT = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
#endif

static constexpr float WIN_REVEAL_SECONDS        = 2.5f;
static constexpr float OPENING_DECISION_SECONDS  = 3.0f;

static std::string formatOpeningDecision(const OpeningDecision& d, bool aiVsAi)
{
    const char* who = aiVsAi
        ? (d.chooser == Seat::First ? "Seat 1" : "Seat 2")
        : "AI";

    std::string action = "Play White";
    switch (d.choice)
    {
        case OpeningChoice::PlaceTwo:
            action = "Place 2 more stones";
            break;
        case OpeningChoice::Keep:
        case OpeningChoice::Swap:
            action = (d.colorTaken == Color::Black) ? "Play Black" : "Play White";
            break;
    }
    return std::string(who) + " chose " + action;
}

Gomoku::Gomoku()
    : _window(sf::VideoMode(WIN_W, WIN_H), "Gomoku", sf::Style::Titlebar | sf::Style::Close)
{
    _window.setFramerateLimit(60);

    _font.loadFromFile(FONT);

    buildMainMenuPage();
    buildRuleDemosPage();
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
        case AppState::RuleDemos:  return _ruleDemos;
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
    const char *modeStr      = _config.aiVsAi ? "AI vs AI"
                             : _config.aiOpponent ? "vs AI"
                             : "hotseat";

    LOG_INFO("CONFIG",
        std::string("mode=") + modeStr
        + "  board="   + std::to_string(_config.boardSize) + "x" + std::to_string(_config.boardSize)
        + "  stone="   + stoneStr  [static_cast<int>(_config.playerColor)]
        + "  opening=" + openingStr[static_cast<int>(_config.openingProtocol)]);
    LOG_SUPPRESS(modeStr, stoneStr[static_cast<int>(_config.playerColor)], openingStr[static_cast<int>(_config.openingProtocol)]);
}

void Gomoku::onBoardSizeSelected(int size)
{
    _config.boardSize = size;
    // Both seats are AI — no human colour to pick.
    if (_config.aiVsAi)
        navigateTo(AppState::Opening);
    else
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
    _awaitingWinScreen = false;
    _statusBanner.clear();

    LOG_INFO("GAME DEBUG", "player actor: " + seatStr(_controller->playerActor().seat) + " " + (_controller->playerActor().color == Color::Black ? "Black" : "White"));
    LOG_INFO("GAME DEBUG", "ai actor: " + seatStr(_controller->aiActor().seat) + " " + (_controller->aiActor().color == Color::Black ? "Black" : "White"));
    LOG_INFO("GAME DEBUG", "current actor: " + seatStr(_controller->currentActor().seat) + " " + (_controller->currentActor().color == Color::Black ? "Black" : "White"));
}

void Gomoku::clearRuleDemo()
{
    _demoHint.clear();
    _demoKeyCell.reset();
}

void Gomoku::startRuleDemo(const RuleDemo& demo)
{
    _config.boardSize       = 19;
    _config.playerColor     = Color::Black;
    _config.openingProtocol = OpeningProtocol::Standard;
    _config.aiOpponent      = false;
    _config.aiVsAi          = false;

    _demoHint = demo.hint;
    _demoKeyCell = Move{ demo.keyCol, demo.keyRow, CellStatus::Empty };

    navigateTo(AppState::Game);
    _controller->seedStandardPosition(makeDemoBoard(demo), demo.toMove);
}

// ── Ghost-colour computation ──────────────────────────────────────────────────

CellStatus Gomoku::computeGhostColor() const
{
    if (!_controller)
        return CellStatus::Empty;
    if (_controller->getColorFromWinningActor().has_value())
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
    if (_controller->aiVsAi())
        return true;
    if (!_controller->aiOpponent())
        return false;
    return _controller->currentActor().seat == _controller->aiActor().seat;
}

bool Gomoku::isGameOver()
{
    if (!_controller)
        return false;

    const auto winner = _controller->getColorFromWinningActor();
    if (!winner.has_value())
        return false;

    if (!_awaitingWinScreen)
    {
        _awaitingWinScreen = true;
        _winRevealClock.restart();
        return true;
    }

    if (_winRevealClock.getElapsedTime().asSeconds() < WIN_REVEAL_SECONDS)
        return true;

    buildWinScreenPage(winner.value(),
                       _controller->blackCaptureCount(),
                       _controller->whiteCaptureCount());
    navigateTo(AppState::GameOver);
    _awaitingWinScreen = false;
    return true;
}

void Gomoku::requestSuggestion()
{
    if (!_controller || _states.top() != AppState::Game)
        return;
    if (_awaitingWinScreen || _controller->getColorFromWinningActor().has_value())
        return;
    if (_statusBanner.blocking())
        return;
    if (_controller->phase() != GamePhase::Standard)
        return;
    // Hints are for humans only — skip on AI turns and in AI vs AI.
    if (_controller->aiVsAi() || (_config.aiOpponent && isAITurn()))
        return;

    _suggestion = _controller->suggestMove();
}

void Gomoku::clearSuggestion()
{
    _suggestion.reset();
}

void Gomoku::announceAiOpeningDecision()
{
    const auto decision = _controller->takeOpeningDecision();
    if (!decision.has_value())
        return;

    _statusBanner.flash(
        formatOpeningDecision(*decision, _controller->aiVsAi()),
        GOLD, OPENING_DECISION_SECONDS);
}

void Gomoku::refreshStatusBanner()
{
    if (_statusBanner.blocking() || !_controller)
        return;

    if (!_demoHint.empty())
    {
        _statusBanner.setPersistent(_demoHint, GOLD);
        return;
    }

    if (_controller->phase() == GamePhase::Standard && !_controller->aiVsAi())
    {
        const bool active = _suggestion.has_value();
        _statusBanner.setPersistent(
            active ? "Suggested move shown  |  press H for another"
                   : "Press H for a move suggestion",
            active ? GOLD : DIM);
    }
    else if (_controller->aiVsAi() && _controller->phase() == GamePhase::Standard)
    {
        _statusBanner.setPersistent("AI vs AI — spectating", DIM);
    }
    else
    {
        _statusBanner.setPersistent("", DIM);
    }
}

void Gomoku::handleEvent(const sf::Event &event, sf::Vector2f mouse)
{
    if (event.type == sf::Event::Closed)
        _window.close();

    // Move suggestion (hint): press H while in a live game.
    if (event.type == sf::Event::KeyPressed
        && event.key.code == sf::Keyboard::H)
    {
        requestSuggestion();
        return;
    }

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

    if (_awaitingWinScreen || _controller->getColorFromWinningActor().has_value())
        return;
    if (_statusBanner.blocking())
        return;


    if (_controller->aiVsAi())
        return;

    switch (_controller->phase())
    {
        case GamePhase::Opening:
        {
            int col = _board->getHoveredCol();
            int row = _board->getHoveredRow();
            
            if (col < 0 || row < 0)
                break;

            if (!_controller->submitOpeningMove(col, row))
                break;

            clearSuggestion();

            if (_controller->phase() == GamePhase::ColorChoice && !isAITurn())
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

            _controller->submitMove(col, row);
            clearSuggestion();
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

    if (_states.top() != AppState::Game || !_controller)
        return;

    if (isGameOver())
        return;

    if (_statusBanner.blocking())
        return;

    if (_config.aiOpponent && isAITurn())
    { 
        _controller->requestAIMove();
        clearSuggestion();

        // Opening may land on ColorChoice. Resolve it now if the AI is the
        // chooser so the banner can announce Keep / Swap / Place 2 this frame.
        if (_controller->phase() == GamePhase::ColorChoice && isAITurn())
            _controller->requestAIMove();
        else if (_controller->phase() == GamePhase::ColorChoice)
            buildColorChoicePage();

        announceAiOpeningDecision();
        isGameOver();
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
        _renderer.renderGame(_window, *_board, *_controller, computeGhostColor(),
                             _suggestion.has_value() ? _suggestion : _demoKeyCell);
        _renderer.renderStats(_window, _font, *_board, *_controller);
        if (_controller->phase() == GamePhase::ColorChoice && !_controller->aiVsAi())
            _renderer.renderColorChoice(_window, _colorChoice);
        refreshStatusBanner();
        _statusBanner.draw(_window, _font);
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
    clearSuggestion();
    clearRuleDemo();
    _awaitingWinScreen = false;
    _statusBanner.clear();
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

    if (_config.aiOpponent)
    {
        std::ostringstream avgStream;
        avgStream << std::fixed << std::setprecision(1);
        if (_config.aiVsAi)
        {
            avgStream << "Black avg: " << _controller->aiMoveAverageMs(Color::Black)
                      << " ms  |  White avg: " << _controller->aiMoveAverageMs(Color::White)
                      << " ms";
        }
        else
        {
            avgStream << "AI avg: "
                      << _controller->aiMoveAverageMs(_controller->aiActor().color)
                      << " ms";
        }
        sf::Text aiAvg = makeText(avgStream.str(), _font, FONT_MD, GOLD);
        aiAvg.setStyle(sf::Text::Bold);
        aiAvg.setPosition(CX, WIN_H * 0.478f);
        _winScreen.addText("aiAvg", aiAvg);
    }

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
        if (auto *t  = page.getText("aiAvg"))  win.draw(*t);
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

    _mainMenu.addItem("playAI", FonctionItem(
        Item("Play vs AI", _font, CX, WIN_H * 0.375f),
        [this]() {
            _config.aiOpponent = true;
            _config.aiVsAi    = false;
            navigateTo(AppState::BoardSize);
        }
    ));
    _mainMenu.addItem("playHotseat", FonctionItem(
        Item("Two Players", _font, CX, WIN_H * 0.460f),
        [this]() {
            _config.aiOpponent = false;
            _config.aiVsAi    = false;
            navigateTo(AppState::RuleDemos);
        }
    ));
    _mainMenu.addItem("playAIvsAI", FonctionItem(
        Item("AI vs AI", _font, CX, WIN_H * 0.545f),
        [this]() {
            _config.aiOpponent = true;
            _config.aiVsAi    = true;
            _config.playerColor = Color::Black; // unused; AI drives both seats
            navigateTo(AppState::BoardSize);
        }
    ));
    _mainMenu.addItem("quit", FonctionItem(
        Item("Quit", _font, CX, WIN_H * 0.630f),
        [this]() { _window.close(); }
    ));

    _mainMenu.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t = page.getText("title"))         win.draw(*t);
        if (auto *r = page.getRectangle("divider"))  win.draw(*r);
        if (auto *t = page.getText("sub"))           win.draw(*t);
        if (auto *fi = page.getItem("playAI"))       fi->item.draw(win);
        if (auto *fi = page.getItem("playHotseat"))  fi->item.draw(win);
        if (auto *fi = page.getItem("playAIvsAI"))   fi->item.draw(win);
        if (auto *fi = page.getItem("quit"))         fi->item.draw(win);
    });
}

void Gomoku::buildRuleDemosPage()
{
    sf::Text title = makeText("GOMOKU", _font, FONT_LG, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(CX, WIN_H * 0.10f);
    _ruleDemos.addText("title", title);

    sf::Text sub = makeText("Two Players — load a prepared position", _font, FONT_MD, DIM);
    sub.setPosition(CX, WIN_H * 0.175f);
    _ruleDemos.addText("sub", sub);

    _ruleDemos.addItem("newGame", FonctionItem(
        Item("New game", _font, CX, WIN_H * 0.30f),
        [this]() {
            clearRuleDemo();
            navigateTo(AppState::BoardSize);
        }
    ));
    _ruleDemos.addItem("doubleThree", FonctionItem(
        Item("Double-three", _font, CX, WIN_H * 0.38f),
        [this]() { startRuleDemo(demoDoubleThree()); }
    ));
    _ruleDemos.addItem("doubleThreeCap", FonctionItem(
        Item("Double-three + capture", _font, CX, WIN_H * 0.46f),
        [this]() { startRuleDemo(demoDoubleThreeCapture()); }
    ));
    _ruleDemos.addItem("breakableFive", FonctionItem(
        Item("Breakable five", _font, CX, WIN_H * 0.54f),
        [this]() { startRuleDemo(demoBreakableFive()); }
    ));
    _ruleDemos.addItem("return", FonctionItem(
        Item("Return", _font, CX, WIN_H * 0.64f),
        [this]() { goBack(); }
    ));

    _ruleDemos.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t  = page.getText("title"))            win.draw(*t);
        if (auto *t  = page.getText("sub"))              win.draw(*t);
        if (auto *fi = page.getItem("newGame"))          fi->item.draw(win);
        if (auto *fi = page.getItem("doubleThree"))      fi->item.draw(win);
        if (auto *fi = page.getItem("doubleThreeCap"))   fi->item.draw(win);
        if (auto *fi = page.getItem("breakableFive"))    fi->item.draw(win);
        if (auto *fi = page.getItem("return"))           fi->item.draw(win);
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
    _boardSize.addItem("return", FonctionItem(
        Item("Return", _font, CX, WIN_H * 0.5625f),
        [this]() { goBack(); }
    ));
    _boardSize.addItem("quit", FonctionItem(
        Item("Quit", _font, CX, WIN_H * 0.6625f),
        [this]() { _window.close(); }
    ));


    _boardSize.setDrawFunction([](MenuPage &page, sf::RenderWindow &win) {
        if (auto *t  = page.getText("title"))  win.draw(*t);
        if (auto *t  = page.getText("sub"))    win.draw(*t);
        if (auto *fi = page.getItem("15x15"))  fi->item.draw(win);
        if (auto *fi = page.getItem("19x19"))  fi->item.draw(win);
        if (auto *fi = page.getItem("return")) fi->item.draw(win);
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
