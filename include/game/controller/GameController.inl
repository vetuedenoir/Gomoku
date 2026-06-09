#include "game/validation/rules/OpeningRules.hpp"
#include "logger/Logger.hpp"
#include <random>
#include <string>

template<typename Traits>
GameController<Traits>::GameController(const GameConfig& config)
    : _state(config.boardSize, config.openingProtocol, config.playerColor)
{
    if (_state.phase == GamePhase::Standard)
        beginNormalPlay();
}

template<typename Traits>
void GameController<Traits>::beginNormalPlay()
{
    const Seat blackSeat = _state.blackSeat.value_or(Seat::First);
    _state.board->setCurrentPlayer(blackSeat);

    Logger::info("PHASE",
        std::string("Standard — Black=") + seatStr(blackSeat)
        + "  White=" + seatStr(_state.whiteSeat.value_or(Seat::Second))
        + "  to move=" + seatStr(_state.board->currentSeat()));
}

template<typename Traits>
Color GameController<Traits>::currentColor() const
{
    const Seat seat = _state.board->currentSeat();
    if (_state.blackSeat.has_value())
        return (_state.blackSeat.value() == seat) ? Color::Black : Color::White;
    return (seat == Seat::First) ? Color::Black : Color::White;
}

template<typename Traits>
bool GameController<Traits>::handleOpeningClick(int col, int row)
{
    if (_state.phase != GamePhase::Opening)
    {
        Logger::warn("CONTROLLER", "handleOpeningClick rejected — not Opening");
        return false;
    }

    const GamePhase prevPhase = _state.phase;
    const CellStatus forced     = _state.nextOpeningColor();
    const Move       m{ col, row, forced };
    if (!commitOpeningMove(_state, m))
        return false;

    if (prevPhase != GamePhase::Standard && _state.phase == GamePhase::Standard)
        beginNormalPlay();

    return true;
}

template<typename Traits>
void GameController<Traits>::resolveColorChoice(bool swapped)
{
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

    const Move move{ col, row, colorToCell(currentColor()) };

    if (!_validator.isLegal(_state, move))
        return MoveResult::Illegal;

    const PlayResult r = _turnController.play(_state, _capturesBlack, _capturesWhite, move);
    if (r.winner)
        _winner = r.winner;

    return r.result;
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
Seat GameController<Traits>::currentActor() const
{
    return _state.currentActor;
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
std::optional<Color> GameController<Traits>::winner() const
{
    return _winner;
}

template<typename Traits>
int GameController<Traits>::captureCount(const Color c) const
{
    return (c == Color::Black) ? _capturesBlack : _capturesWhite;
}
