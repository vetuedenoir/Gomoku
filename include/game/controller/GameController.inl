#include "game/validation/rules/OpeningRules.hpp"
#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"
#include <random>
#include <string>

template<typename Traits>
GameController<Traits>::GameController(const GameConfig& config)
    : _board(std::make_unique<GameBoard>(config.boardSize, Color::Black)),
      _opening(config.openingProtocol)
{
    if (config.playerColor == Color::Black)
    {
        _aiActor.color = Color::White;
        _aiActor.seat = Seat::Second;
        _playerActor.color = Color::Black;
        _playerActor.seat = Seat::First;
        _masterAI.setAIColor(Color::White);
    }
    else
    {
        _aiActor.color = Color::Black;
        _aiActor.seat = Seat::First;
        _playerActor.color = Color::White;
        _playerActor.seat = Seat::Second;
        _masterAI.setAIColor(Color::Black);
    }

    _phase = _opening.openingSteps.empty() ? GamePhase::Standard : GamePhase::Opening;

    if (_phase == GamePhase::Standard)
        _currentActor = actorWithColor(Color::Black);
    else
        setCurrentActorForSeat(Seat::First);

    Logger::info("CONTROLLER",
        std::string("phase=") + (_phase == GamePhase::Standard ? "Standard"
            : _phase == GamePhase::Opening ? "Opening" : "ColorChoice")
        + "  current=" + seatStr(_currentActor.seat)
        + " (" + (_currentActor.color == Color::Black ? "Black" : "White") + ")");
}

template<typename Traits>
ValidationContext GameController<Traits>::validationContext() const
{
    return { *_board, _opening };
}

template<typename Traits>
void GameController<Traits>::logPhaseTransition(GamePhase from, GamePhase to) const
{
    auto str = [](GamePhase p) {
        switch (p)
        {
            case GamePhase::Opening:     return "Opening";
            case GamePhase::ColorChoice: return "ColorChoice";
            case GamePhase::Standard:    return "Standard";
        }
        return "Unknown";
    };
    Logger::info("PHASE", std::string(str(from)) + " → " + str(to));
}

template<typename Traits>
void GameController<Traits>::assignColorsAfterChoice(bool swapped)
{
    if (swapped)
    {
        std::swap(_playerActor.color, _aiActor.color);
        std::swap(_playerActor.seat, _aiActor.seat);
        _masterAI.setAIColor(_aiActor.color);
    }

    Logger::info("RESOLVE",
        "Player → " + seatStr(_playerActor.seat)
        + "  |  AI → " + seatStr(_aiActor.seat)
        + "  |  swapped=" + (swapped ? "true" : "false"));
}

template<typename Traits>
Actor GameController<Traits>::actorWithColor(Color color) const
{
    return (_playerActor.color == color) ? _playerActor : _aiActor;
}

template<typename Traits>
Color GameController<Traits>::colorForSeat(Seat seat) const
{
    return (seat == _playerActor.seat) ? _playerActor.color : _aiActor.color;
}

template<typename Traits>
void GameController<Traits>::setCurrentActorForSeat(Seat seat)
{
    _currentActor = { seat, colorForSeat(seat) };
}

template<typename Traits>
void GameController<Traits>::syncCurrentActorColor()
{
    _currentActor.color = colorForSeat(_currentActor.seat);
}

template<typename Traits>
void GameController<Traits>::syncBoardCurrentColor()
{
    _board->setCurrentColor(_currentActor.color);
}

template<typename Traits>
void GameController<Traits>::beginNormalPlay()
{
    syncBoardCurrentColor();

    Logger::info("PHASE BEGIN NORMAL PLAY",
        std::string("Standard — Player=") + seatStr(_playerActor.seat)
        + "  AI=" + seatStr(_aiActor.seat)
        + "  to move=" + seatStr(_currentActor.seat)
        + " (" + (_currentActor.color == Color::Black ? "Black" : "White") + ")");
}

template<typename Traits>
void GameController<Traits>::enterStandardPhase()
{
    const GamePhase prev = _phase;
    _phase = GamePhase::Standard;
    logPhaseTransition(prev, _phase);
    beginNormalPlay();
}

template<typename Traits>
void GameController<Traits>::applyOpeningResult(const OpeningCommitResult& result)
{
    if (result.nextSeat.has_value())
        setCurrentActorForSeat(*result.nextSeat);

    switch (result.event)
    {
        case OpeningEvent::StepContinues:
        case OpeningEvent::NextStep:
            break;

        case OpeningEvent::ColorChoice:
        {
            const GamePhase prev = _phase;
            _phase = GamePhase::ColorChoice;
            logPhaseTransition(prev, _phase);
            break;
        }

        case OpeningEvent::Finished:
            enterStandardPhase();
            break;
    }
}

template<typename Traits>
void GameController<Traits>::passTurn()
{
    _currentActor = (_currentActor.seat == _playerActor.seat) ? _aiActor : _playerActor;
    syncBoardCurrentColor();
}

template<typename Traits>
Color GameController<Traits>::currentColor() const
{
    return _currentActor.color;
}

template<typename Traits>
bool GameController<Traits>::handleOpeningClick(int col, int row)
{
    const CellStatus forced = _opening.nextOpeningColor();
    const Move       m{ col, row, forced };

    const OpeningCommitResult result = commitOpeningMove(_opening, *_board, m);
    if (!result.success)
        return false;

    applyOpeningResult(result);
    return true;
}

template<typename Traits>
void GameController<Traits>::resolveColorChoice(bool swapped)
{
    Logger::info("CHOICE",
        seatStr(_currentActor.seat)
        + (swapped ? " swapped → takes opposite colour"
                   : " keeps default colour"));

    // Swap interverts player/ai actors, not currentActor seat.
    assignColorsAfterChoice(swapped);
    syncCurrentActorColor();

    // Chooser finished their action → pass to opposite colour for first Standard move.
    passTurn();
    enterStandardPhase();
}

template<typename Traits>
void GameController<Traits>::continueOpeningPlacement()
{
    const GamePhase prev = _phase;
    _opening.continueOpeningPlacement();
    _phase = GamePhase::Opening;
    setCurrentActorForSeat(Seat::Second);
    logPhaseTransition(prev, _phase);
}

template<typename Traits>
MoveResult GameController<Traits>::submitMove(int col, int row)
{
    if (_winner.has_value())
        return MoveResult::Illegal;
    if (_phase != GamePhase::Standard)
        return MoveResult::Illegal;

    const Color moverColor = currentColor();
    const Move  move{ col, row, colorToCell(moverColor) };

    if (!_validator.isLegal(validationContext(), _phase, moverColor, move))
        return MoveResult::Illegal;

    t_BWBoard<Traits> bb = GameBoard_to_bitboard<Traits>(*_board);
    const TurnOutcome<Traits> outcome =
        _turnController.play(bb, move, _capturesBlack, _capturesWhite);

    _board->placeStoneOfColor(move.col, move.row, move.forcedColor);
    bb_for_each_bit<Traits>(outcome.capturedMask, [this](int x, int y) {
        _board->clearCell(x, y);
    });

    if (moverColor == Color::Black)
        _capturesBlack += outcome.capturesAdded;
    else
        _capturesWhite += outcome.capturesAdded;

    if (outcome.result == MoveResult::Win && outcome.winnerByColor.has_value())
        _winner = outcome.winnerByColor;

    if (outcome.result == MoveResult::Win)
        return MoveResult::Win;

    passTurn();

    return MoveResult::Ok;
}

template<typename Traits>
std::optional<Move> GameController<Traits>::requestAIMove()
{
    if (_winner.has_value())
    {
        Logger::info("AI", "requestAIMove — game over");
        return std::nullopt;
    }

    if (_phase == GamePhase::ColorChoice)
    {
        static std::mt19937 rng{std::random_device{}()};
        bool swapped = std::uniform_int_distribution<int>(0, 1)(rng);
        Logger::info("AI", std::string("requestAIMove — ColorChoice, swapped=")
                           + (swapped ? "true" : "false"));
        resolveColorChoice(swapped);
        return std::nullopt;
    }

    if (_phase == GamePhase::Opening)
    {
        _tracker.start();

        const std::vector<Move> candidates = getLegalOpeningMoves(_opening, *_board);

        const Move& m = candidates.front();
        if (!handleOpeningClick(m.col, m.row))
        {
            Logger::warn("AI", "requestAIMove — opening click rejected");
            return std::nullopt;
        }

        _tracker.stop();

        return m;
    }

    if (_phase == GamePhase::Standard)
    {
        SearchPosition<Traits> pos = SearchPosition<Traits>::fromBoard(*_board);
        _tracker.start();
        auto [col, row] = _masterAI.findBestMove(pos, currentColor());
       
        _tracker.stop();

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
    return *_board;
}

template<typename Traits>
GamePhase GameController<Traits>::phase() const
{
    return _phase;
}

template<typename Traits>
Actor GameController<Traits>::playerActor() const
{
    return _playerActor;
}

template<typename Traits>
Actor GameController<Traits>::aiActor() const
{
    return _aiActor;
}

template<typename Traits>
Actor GameController<Traits>::currentActor() const
{
    return _currentActor;
}

template<typename Traits>
CellStatus GameController<Traits>::nextOpeningColor() const
{
    return _opening.nextOpeningColor();
}

template<typename Traits>
OpeningProtocol GameController<Traits>::openingProtocol() const
{
    return _opening.openingProtocol;
}

template<typename Traits>
int GameController<Traits>::stepIdx() const
{
    return _opening.stepIdx;
}

template<typename Traits>
std::optional<Color> GameController<Traits>::getColorFromWinningActor() const
{
    return _winner;
}

template<typename Traits>
int GameController<Traits>::captureCount(const Color c) const
{
    return (c == Color::Black) ? _capturesBlack : _capturesWhite;
}

template<typename Traits>
double GameController<Traits>::aiMoveLastMs() const
{
    return _tracker.last();
}

template<typename Traits>
double GameController<Traits>::aiMoveAverageMs() const
{
    return _tracker.average();
}
