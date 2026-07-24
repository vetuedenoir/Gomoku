#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"
#include <random>
#include <string>

template<typename Traits>
GameController<Traits>::GameController(const GameConfig& config)
    : _board(std::make_unique<GameBoard>(config.boardSize, Color::Black)),
      _opening(config.openingProtocol)
{
    // Colours are bound to seats and never move: First is always Black, Second always White.
    _colorBySeat = { Color::Black, Color::White };
    // The human picked their colour at game creation; the AI takes the opposite seat.
    _aiSeat = (config.playerColor == Color::Black) ? Seat::Second : Seat::First;

    _aiOpponent = config.aiOpponent;

    _masterAI.setAIColor(aiColor());

    _phase = _opening.isComplete() ? GamePhase::Standard : GamePhase::Opening;

    _currentSeat = Seat::First;

    Logger::info("CONTROLLER",
        std::string("phase=") + (_phase == GamePhase::Standard ? "Standard"
            : _phase == GamePhase::Opening ? "Opening" : "ColorChoice")
        + "  current=" + seatStr(_currentSeat)
        + " (" + (colorOf(_currentSeat) == Color::Black ? "Black" : "White") + ")");
}

template<typename Traits>
Color GameController<Traits>::colorOf(Seat s) const
{
    return _colorBySeat[static_cast<int>(s)];
}

template<typename Traits>
Seat GameController<Traits>::seatOf(Color c) const
{
    return (colorOf(Seat::First) == c) ? Seat::First : Seat::Second;
}

template<typename Traits>
Color GameController<Traits>::aiColor() const
{
    return colorOf(_aiSeat);
}

template<typename Traits>
Color GameController<Traits>::playerColor() const
{
    return colorOf(otherSeat(_aiSeat));
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
    // A swap only trades which seat the AI occupies; colours stay bound to seats.
    if (swapped)
    {
        _aiSeat = otherSeat(_aiSeat);
        _masterAI.setAIColor(aiColor());
    }

    Logger::info("RESOLVE",
        "Player → " + seatStr(otherSeat(_aiSeat))
        + "  |  AI → " + seatStr(_aiSeat)
        + "  |  swapped=" + (swapped ? "true" : "false"));
}

template<typename Traits>
void GameController<Traits>::beginNormalPlay()
{
    _board->setCurrentColor(colorOf(_currentSeat));

    Logger::info("PHASE BEGIN NORMAL PLAY",
        std::string("Standard — Player=") + seatStr(otherSeat(_aiSeat))
        + "  AI=" + seatStr(_aiSeat)
        + "  to move=" + seatStr(_currentSeat)
        + " (" + (colorOf(_currentSeat) == Color::Black ? "Black" : "White") + ")");
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
        _currentSeat = *result.nextSeat;

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
    _currentSeat = otherSeat(_currentSeat);
    _board->setCurrentColor(colorOf(_currentSeat));
}

template<typename Traits>
Color GameController<Traits>::currentColor() const
{
    return colorOf(_currentSeat);
}

template<typename Traits>
bool GameController<Traits>::submitOpeningMove(int col, int row)
{
    const CellStatus forced = _opening.nextColor();
    
    const Move       m{ col, row, forced };

    const OpeningCommitResult result = _opening.commit(*_board, m);
    
    if (!result.success)
        return false;

    applyOpeningResult(result);

    return true;
}

template<typename Traits>
void GameController<Traits>::resolveColorChoice(bool swapped)
{
    Logger::info("CHOICE",
        seatStr(_currentSeat)
        + (swapped ? " swapped → takes opposite colour"
                   : " keeps default colour"));

    assignColorsAfterChoice(swapped);

    _currentSeat = seatOf(Color::White);
   
    enterStandardPhase();
}

template<typename Traits>
void GameController<Traits>::continueOpeningPlacement()
{
    const GamePhase prev = _phase;
    _opening.continuePlacement();
    _phase = GamePhase::Opening;

    // The seat that places the extra stones is dictated by the current opening step, not a literal.
    _currentSeat = _opening.currentStep().actor;

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

    if (!_validator.isLegal(*_board, moverColor, move))
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


        const std::vector<Move> candidates = _opening.legalMoves(*_board);

        static std::mt19937 rng{std::random_device{}()};
        
        std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
        
        const Move m = candidates[pick(rng)];

        submitOpeningMove(m.col, m.row);

        return Move{ m.col, m.row, colorToCell(currentColor()) };
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
        
        submitMove(col, row);
        
        return Move{ static_cast<int>(col), static_cast<int>(row), colorToCell(currentColor()) };
    }

    return std::nullopt;
}

template<typename Traits>
std::optional<Move> GameController<Traits>::suggestMove()
{
    if (_winner.has_value() || _phase != GamePhase::Standard)
        return std::nullopt;

    const Color side = currentColor();

    SearchPosition<Traits> pos = SearchPosition<Traits>::fromBoard(*_board);

    const auto [col, row] = _masterAI.findBestMove(pos, side);

    if (col == -1)
        return std::nullopt;

    return Move{ static_cast<int>(col), static_cast<int>(row), colorToCell(side) };
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
    return { otherSeat(_aiSeat), playerColor() };
}

template<typename Traits>
Actor GameController<Traits>::aiActor() const
{
    return { _aiSeat, aiColor() };
}

template<typename Traits>
Actor GameController<Traits>::currentActor() const
{
    return { _currentSeat, colorOf(_currentSeat) };
}

template<typename Traits>
bool GameController<Traits>::aiOpponent() const
{
    return _aiOpponent;
}

template<typename Traits>
CellStatus GameController<Traits>::nextOpeningColor() const
{
    return _opening.nextColor();
}

template<typename Traits>
OpeningProtocol GameController<Traits>::openingProtocol() const
{
    return _opening.protocol();
}

template<typename Traits>
int GameController<Traits>::stepIdx() const
{
    return _opening.stepIndex();
}

template<typename Traits>
std::optional<Color> GameController<Traits>::getColorFromWinningActor() const
{
    return _winner;
}

template<typename Traits>
int GameController<Traits>::blackCaptureCount() const
{
    return _capturesBlack;
}

template<typename Traits>
int GameController<Traits>::whiteCaptureCount() const
{
    return _capturesWhite;
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