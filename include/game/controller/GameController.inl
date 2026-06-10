#include "game/validation/rules/OpeningRules.hpp"
#include "logger/Logger.hpp"
#include <random>
#include <string>

template<typename Traits>
GameController<Traits>::GameController(const GameConfig& config)
    : _state(config.boardSize, config.openingProtocol)
{
    if (_state.phase == GamePhase::Standard)
    {
        assignDefaultColors();
        beginNormalPlay();
    }
}

template<typename Traits>
void GameController<Traits>::assignDefaultColors()
{
    _blackActor  = {Seat::First, Color::Black};
    _whiteActor  = {Seat::Second, Color::White};
}

template<typename Traits>
void GameController<Traits>::assignColorsAfterChoice(bool swapped)
{
    if (!swapped)
    {
        _blackActor = {Seat::First, Color::Black};
        _whiteActor = {Seat::Second, Color::White};
    }
    else
    {
        _blackActor = {Seat::Second, Color::Black};
        _whiteActor = {Seat::First, Color::White};
    }

    Logger::info("RESOLVE",
        "Black → " + seatStr(_blackActor.seat)
        + "  |  White → " + seatStr(_whiteActor.seat));
}

template<typename Traits>
Color GameController<Traits>::colorForSeat(Seat seat) const
{
    return (seat == _blackActor.seat) ? Color::Black : Color::White;
}

template<typename Traits>
void GameController<Traits>::syncBoardCurrentColor()
{
    _state.board->setCurrentColor(_currentActor.color);
}

template<typename Traits>
void GameController<Traits>::beginNormalPlay()
{
    // TODO: verify this on swap openings
    _currentActor = _blackActor;
    syncBoardCurrentColor();

    Logger::info("PHASE",
        std::string("Standard — Black=") + seatStr(_blackActor.seat)
        + "  White=" + seatStr(_whiteActor.seat)
        + "  to move=" + seatStr(_currentActor.seat)
        + " (" + (_currentActor.color == Color::Black ? "Black" : "White") + ")");
}

template<typename Traits>
void GameController<Traits>::switchActor()
{
    _currentActor = (_currentActor.seat == _blackActor.seat) ? _whiteActor : _blackActor;
    syncBoardCurrentColor();
}

template<typename Traits>
Color GameController<Traits>::currentColor() const
{
    if (_state.phase == GamePhase::Standard)
        return _currentActor.color;
    return colorForSeat(_state.currentActor);
}

template<typename Traits>
bool GameController<Traits>::handleOpeningClick(int col, int row)
{
    const GamePhase prevPhase = _state.phase;
    const CellStatus forced   = _state.nextOpeningColor();
    const Move       m{ col, row, forced };
    if (!commitOpeningMove(_state, m))
        return false;

    if (prevPhase != GamePhase::Standard && _state.phase == GamePhase::Standard)
    {
        assignDefaultColors();
        beginNormalPlay();
    }

    return true;
}

template<typename Traits>
void GameController<Traits>::resolveColorChoice(bool swapped)
{
    assignColorsAfterChoice(swapped);
    // TODO: rename this function
    _state.resolveColorChoice(swapped);
    beginNormalPlay();
}

template<typename Traits>
void GameController<Traits>::continueOpeningPlacement()
{
    _state.continueOpeningPlacement();
}

template<typename Traits>
MoveResult GameController<Traits>::submitMove(int col, int row)
{
    Move move{ col, row, colorToCell(currentColor()) };

    if (!_validator.isLegal(_state, move))
        return MoveResult::Illegal;

    const TurnOutcome outcome = _turnController.play(_state, _capturesBlack, _capturesWhite, move);
    if (outcome.result == MoveResult::Win)
        _winner = { outcome.winnerByColor.value(), outcome.capturesAdded };

    _capturesBlack += outcome.capturesAdded;
    _capturesWhite += outcome.capturesAdded;

    return MoveResult::Illegal;
}

template<typename Traits>
std::optional<Move> GameController<Traits>::requestAIMove()
{
    if (_winner.has_value())
    {
        Logger::info("AI", "requestAIMove — game over");
        return std::nullopt;
    }

    if (_state.phase == GamePhase::ColorChoice)
    {
        static std::mt19937 rng{std::random_device{}()};
        bool swapped = std::uniform_int_distribution<int>(0, 1)(rng);
        Logger::info("AI", std::string("requestAIMove — ColorChoice, swapped=")
                           + (swapped ? "true" : "false"));
        resolveColorChoice(swapped);
        return std::nullopt;
    }

    Logger::info("AI", "requestAIMove — in progress");

    if (_state.phase == GamePhase::Opening)
    {
        const std::vector<Move> candidates = getLegalOpeningMoves(_state);
        if (candidates.empty())
        {
            Logger::warn("AI", "requestAIMove — no legal opening moves");
            return std::nullopt;
        }
        Logger::debug("AI", "[requestAIMove/Opening] candidates: " + std::to_string(candidates.size()));
        
        std::string candidatesStr = "";
        for (const auto& c : candidates)
            candidatesStr += "  (" + std::to_string(c.col) + ", " + std::to_string(c.row) + "), ";

        Logger::debug("AI", "[requestAIMove/Opening] candidates: " + candidatesStr);
        const Move& m = candidates.front();
        if (!handleOpeningClick(m.col, m.row))
        {
            Logger::warn("AI", "requestAIMove — opening click rejected");
            return std::nullopt;
        }
        return m;
    }

    if (_state.phase == GamePhase::Standard)
    {
        SearchPosition<Traits> pos = SearchPosition<Traits>::fromBoard(*_state.board);
        auto [col, row] = _masterAI.findBestMove(pos, currentColor());
        if (col == -1)
        {
            Logger::warn("AI", "requestAIMove — no standard move found");
            return std::nullopt;
        }
        const Move m{ col, row, colorToCell(currentColor()) };
        submitMove(col, row);
        return m;
    }

    return std::nullopt;
}

template<typename Traits>
const GameBoard& GameController<Traits>::visualBoard() const
{
    return *_state.board;
}

template<typename Traits>
GamePhase GameController<Traits>::phase() const
{
    return _state.phase;
}

template<typename Traits>
Actor GameController<Traits>::currentActor() const
{
    if (_state.phase == GamePhase::Standard)
        return _currentActor;
    // TODO: actor is suppose to be on game controller, not on state
    return {_state.currentActor, colorForSeat(_state.currentActor)};
}

template<typename Traits>
CellStatus GameController<Traits>::nextOpeningColor() const
{
    return _state.nextOpeningColor();
}

template<typename Traits>
OpeningProtocol GameController<Traits>::openingProtocol() const
{
    return _state.openingProtocol;
}

template<typename Traits>
int GameController<Traits>::stepIdx() const
{
    return _state.stepIdx;
}

template<typename Traits>
std::optional<Color> GameController<Traits>::getColorFromWinningActor() const
{
    if (!_winner.has_value())
        return std::nullopt;
    return _winner->color;
}

template<typename Traits>
int GameController<Traits>::captureCount(const Color c) const
{
    return (c == Color::Black) ? _capturesBlack : _capturesWhite;
}
