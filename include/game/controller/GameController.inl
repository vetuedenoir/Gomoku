#include "game/rules/OpeningRules.hpp"
#include "logger/Logger.hpp"
#include <string>

namespace
{

CellStatus colorToCell(Color c)
{
    return (c == Color::Black) ? CellStatus::Black : CellStatus::White;
}

} // namespace

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
        Logger::info("AI", "requestAIMove — ColorChoice, no move");
        return std::nullopt;
    }

    const std::vector<Move> candidates = _validator.getLegalMoves(_state);

    if (_state.phase == GamePhase::Opening)
    {
        for (const Move& m : candidates)
        {
            if (handleOpeningClick(m.col, m.row))
                return m;
        }
        Logger::warn("AI", "requestAIMove — no opening move committed");
        return std::nullopt;
    }

    if (_state.phase == GamePhase::Standard)
    {
        for (const Move& m : candidates)
        {
            const MoveResult r = submitMove(m.col, m.row);
            if (r == MoveResult::Ok || r == MoveResult::Win)
                return m;
        }
        Logger::warn("AI", "requestAIMove — no standard move committed");
        return std::nullopt;
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
int GameController<Traits>::captureCount(Color c) const
{
    return (c == Color::Black) ? _capturesBlack : _capturesWhite;
}
