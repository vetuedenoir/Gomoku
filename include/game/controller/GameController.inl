#include "game/validation/rules/OpeningRules.hpp"
#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"
#include <random>
#include <string>

template<typename Traits>
GameController<Traits>::GameController(const GameConfig& config)
    : _state(config.boardSize, config.openingProtocol)
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

template<typename Traits>
void GameController<Traits>::assignDefaultColors()
{
    _blackActor  = {Seat::First, Color::Black};
    _whiteActor  = {Seat::Second, Color::White};
}

template<typename Traits>
void GameController<Traits>::assignColorsAfterChoice(bool swapped)
{
    Logger::info("BEFORE RESOLVE", "swapped: " + (std::string(swapped ? "true" : "false")));
    Logger::info("BEFORE RESOLVE", "player actor: " + seatStr(_playerActor.seat) + " " + (_playerActor.color == Color::Black ? "Black" : "White"));
    Logger::info("BEFORE RESOLVE", "ai actor: " + seatStr(_aiActor.seat) + " " + (_aiActor.color == Color::Black ? "Black" : "White"));



    if (swapped) {
        std::swap(_playerActor.color, _aiActor.color);
        std::swap(_playerActor.seat, _aiActor.seat);
    }

    Logger::info("RESOLVE",
        "Black → " + seatStr(_blackActor.seat)
        + "  |  White → " + seatStr(_whiteActor.seat));
}

template<typename Traits>
Color GameController<Traits>::colorForSeat(Seat seat) const
{
    return (seat == _playerActor.seat) ? _playerActor.color : _aiActor.color;
}

template<typename Traits>
void GameController<Traits>::syncBoardCurrentColor()
{
    _state.board->setCurrentColor(_currentActor.color);
}

template<typename Traits>
void GameController<Traits>::beginNormalPlay()
{
    syncBoardCurrentColor();

    Logger::info("PHASE",
        std::string("Standard — Player=") + seatStr(_playerActor.seat)
        + "  AI=" + seatStr(_aiActor.seat)
        + "  to move=" + seatStr(_currentActor.seat)
        + " (" + (_currentActor.color == Color::Black ? "Black" : "White") + ")");
}

template<typename Traits>
void GameController<Traits>::switchActor()
{
    _currentActor = (_currentActor.seat == _playerActor.seat) ? _aiActor : _playerActor;
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
void GameController<Traits>::syncCurrentActorFromState()
{
    _currentActor = {_state.currentActor, colorForSeat(_state.currentActor)};
}

template<typename Traits>
bool GameController<Traits>::handleOpeningClick(int col, int row)
{
    const GamePhase prevPhase = _state.phase;
    const CellStatus forced   = _state.nextOpeningColor();
    const Move       m{ col, row, forced };
    if (!commitOpeningMove(_state, m))
        return false;

    
    Logger::debug("BEFORE SYNC DEBUG CURRENT ACTOR CONTROLLER", "currentActor: " + seatStr(_currentActor.seat) + " " + (_currentActor.color == Color::Black ? "Black" : "White"));
    Logger::debug("BEFORE SYNC DEBUG CURRENT ACTOR STATE",
        std::string("currentActor: ") + seatStr(_state.currentActor) + " "
        + (colorForSeat(_state.currentActor) == Color::Black ? "Black" : "White"));

    syncCurrentActorFromState();

    Logger::debug("AFTER SYNC DEBUG CURRENT ACTOR CONTROLLER", "currentActor: " + seatStr(_currentActor.seat) + " " + (_currentActor.color == Color::Black ? "Black" : "White"));
    Logger::debug("AFTER SYNC DEBUG CURRENT ACTOR STATE",
        std::string("currentActor: ") + seatStr(_state.currentActor) + " "
        + (colorForSeat(_state.currentActor) == Color::Black ? "Black" : "White"));
    // // else if (state.stepIdx < (int)state.openingSteps.size())
    // // {
    // //     state.currentActor = toSeat(state.openingSteps[state.stepIdx].actor);
    // //     Logger::debug("OPENING",
    // //         "next step actor → " + seatStr(state.currentActor));
    // // }
    // // else if (state.stepIdx >= (int)state.openingSteps.size())
    // // {
    // //     const GamePhase prev = state.phase;
    // //     state.phase = GamePhase::Standard;


    // //     Logger::info("PHASE HERE",
    // //         std::string(phaseStr(prev)) + " → " + phaseStr(state.phase)
    // //         + "  (opening complete — colour assignment handled by GameController)");
    // // }

    if (prevPhase != GamePhase::Standard && _state.phase == GamePhase::Standard)
    {
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
    if (_winner.has_value())
        return MoveResult::Illegal;
    if (_state.phase != GamePhase::Standard)
        return MoveResult::Illegal;

    const Color moverColor = currentColor();
    const Move  move{ col, row, colorToCell(moverColor) };

    if (!_validator.isLegal(_state, move))
        return MoveResult::Illegal;

    t_BWBoard<Traits> bb = GameBoard_to_bitboard<Traits>(*_state.board);
    const TurnOutcome<Traits> outcome =
        _turnController.play(bb, move, _capturesBlack, _capturesWhite);

    _state.board->placeStoneOfColor(move.col, move.row, move.forcedColor);
    bb_for_each_bit<Traits>(outcome.capturedMask, [this](int x, int y) {
        _state.board->clearCell(x, y);
    });

    if (moverColor == Color::Black)
        _capturesBlack += outcome.capturesAdded;
    else
        _capturesWhite += outcome.capturesAdded;

    if (outcome.result == MoveResult::Win && outcome.winnerByColor.has_value())
        _winner = outcome.winnerByColor;

    if (outcome.result == MoveResult::Win)
        return MoveResult::Win;

    switchActor();
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
        // Logger::debug("AI", "[requestAIMove/Opening] candidates: " + std::to_string(candidates.size()));
        
        std::string candidatesStr = "";
        for (const auto& c : candidates)
            candidatesStr += "  (" + std::to_string(c.col) + ", " + std::to_string(c.row) + "), ";

        // Logger::debug("AI", "[requestAIMove/Opening] candidates: " + candidatesStr);
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
    return _winner;
}

template<typename Traits>
int GameController<Traits>::captureCount(const Color c) const
{
    return (c == Color::Black) ? _capturesBlack : _capturesWhite;
}
