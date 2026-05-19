#include "game/rules/OpeningRules.hpp"
#include "game/rules/WinDetector.hpp"
#include "bitboard/bitboard.hpp"
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
    : _state(config.boardSize, config.openingRule, config.playerStoneColor)
{
    if (_state.phase == GamePhase::NormalPlay)
        onEnteredNormalPlay();
}

template<typename Traits>
void GameController<Traits>::onEnteredNormalPlay()
{
    const Seat blackSeat = _state.blackSeat.value_or(Seat::First);
    _state.board->setCurrentPlayer(blackSeat);

    Logger::info("PHASE",
        std::string("NormalPlay — Black=") + seatStr(blackSeat)
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
MoveResult GameController<Traits>::submitMoveImpl(int col, int row)
{
    const Color      color     = currentColor();
    const CellStatus cellColor = colorToCell(color);
    const char*      colorStr  = (color == Color::Black) ? "Black" : "White";

    const Move move{ col, row, cellColor };
    if (!_validator.isLegal(_state, move))
        return MoveResult::Illegal;

    t_BWBoard<Traits> bb = GameBoard_to_bitboard<Traits>(*_state.board);

    typename Traits::Bitboard capturedMask = {};
    detect_captures<Traits>(bb, col, row, color, capturedMask);

    const int newCaptures = popcount_bb_generic<Traits>(capturedMask);
    if (color == Color::Black)
        _capturesBlack += newCaptures;
    else
        _capturesWhite += newCaptures;

    set_bb_generic<Traits>(
        (color == Color::Black) ? bb.black : bb.white, col, row);
    apply_captures<Traits>(bb, capturedMask, color);

    _state.board->placeStoneOfColor(col, row, cellColor);
    bb_for_each_bit<Traits>(capturedMask, [this](int x, int y) {
        _state.board->clearCell(x, y);
    });

    Logger::debug("CONTROLLER",
        std::string(colorStr) + " → (" + std::to_string(col) + "," + std::to_string(row)
        + ") ✓" + (newCaptures > 0 ? " captures=" + std::to_string(newCaptures) : ""));

    const int moverCaptures =
        (color == Color::Black) ? _capturesBlack : _capturesWhite;
    if (moverCaptures >= 10)
    {
        _winner = color;
        Logger::info("CONTROLLER", std::string(colorStr) + " wins by capture!");
        _state.board->switchPlayer();
        return MoveResult::Win;
    }

    if (isWinAfterMove<Traits>(bb, color, col, row))
    {
        _winner = color;
        Logger::info("CONTROLLER", std::string(colorStr) + " wins by alignment!");
        _state.board->switchPlayer();
        return MoveResult::Win;
    }

    _state.board->switchPlayer();
    return MoveResult::Ok;
}

template<typename Traits>
MoveResult GameController<Traits>::submitMove(int col, int row)
{
    if (_winner.has_value())
        return MoveResult::Illegal;

    if (_state.phase != GamePhase::NormalPlay)
    {
        Logger::warn("CONTROLLER", "submitMove rejected — not NormalPlay");
        return MoveResult::Illegal;
    }

    return submitMoveImpl(col, row);
}

template<typename Traits>
bool GameController<Traits>::handleOpeningClick(int col, int row)
{
    if (_state.phase != GamePhase::OpeningPlacement)
    {
        Logger::warn("CONTROLLER", "handleOpeningClick rejected — not OpeningPlacement");
        return false;
    }

    const GamePhase prevPhase = _state.phase;
    const CellStatus forced   = _state.nextOpeningColor();
    const Move       m{ col, row, forced };
    if (!commitOpeningMove(_state, m))
        return false;

    if (prevPhase != GamePhase::NormalPlay && _state.phase == GamePhase::NormalPlay)
        onEnteredNormalPlay();

    return true;
}

template<typename Traits>
void GameController<Traits>::resolveColorChoice(bool swapped)
{
    _state.resolveColorChoice(swapped);
    onEnteredNormalPlay();
}

template<typename Traits>
void GameController<Traits>::continueOpeningPlacement()
{
    _state.continueOpeningPlacement();
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

    const std::vector<Move> candidates = _validator.legalMoves(_state);
    Logger::info("AI",
        "requestAIMove — " + std::to_string(candidates.size()) + " candidates");

    if (_state.phase == GamePhase::OpeningPlacement)
    {
        for (const Move& m : candidates)
        {
            if (handleOpeningClick(m.col, m.row))
            {
                Logger::debug("AI",
                    "committed opening (" + std::to_string(m.col) + ","
                    + std::to_string(m.row) + ")");
                return m;
            }
        }
        Logger::warn("AI", "requestAIMove — no opening move committed");
        return std::nullopt;
    }

    if (_state.phase == GamePhase::NormalPlay)
    {
        for (const Move& m : candidates)
        {
            const MoveResult r = submitMove(m.col, m.row);
            if (r == MoveResult::Ok || r == MoveResult::Win)
            {
                Logger::debug("AI",
                    "committed normal (" + std::to_string(m.col) + ","
                    + std::to_string(m.row) + ")");
                return m;
            }
        }
        Logger::warn("AI", "requestAIMove — no normal move committed");
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
OpeningRule GameController<Traits>::openingRule() const
{
    return _state.openingRule;
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
