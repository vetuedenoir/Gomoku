#include "game/GameController.hpp"
#include "game/OpeningScript.hpp"
#include "bitboard/bitboard.hpp"
#include "bitboard/pattern.hpp"
#include "logger/Logger.hpp"
#include <cstring>

// Defined in src/bitboard/pattern.cpp but not exposed in the public header.
void build_lookup_table5(t_PatternList5 lookup_table5[361][4]);

static CellStatus colorToCell(Color c)
{
    return (c == Color::Black) ? CellStatus::Black : CellStatus::White;
}

// ── Construction ─────────────────────────────────────────────────────────────

GameController::GameController(const GameConfig& config)
    : _state(config.boardSize, config.openingRule, config.playerStoneColor)
{}

// ── Private helpers ───────────────────────────────────────────────────────────

void GameController::ensurePosition()
{
    if (_position.has_value())
        return;
    _position = SearchPosition::fromBoard(_state.board);
    // Note: fromBoard() maps Seat::First → Black.  For swap-based openings
    // that reassign colours, sideToMove may need correction (TODO Phase 2).
}

Color GameController::currentColor() const
{
    Seat seat = _state.board.currentSeat();
    if (_state.blackSeat.has_value())
        return (_state.blackSeat.value() == seat) ? Color::Black : Color::White;
    return (seat == Seat::First) ? Color::Black : Color::White;
}

bool GameController::checkWin(const t_BWBoard19& bb, Color color, int col, int row)
{
    static t_PatternList5 table[361][4];   // zero-init guaranteed by static storage
    static bool           ready = false;
    if (!ready)
    {
        std::memset(table, 0, sizeof(table));
        build_lookup_table5(table);
        ready = true;
    }
    const bitboard19& plane = (color == Color::Black) ? bb.black : bb.white;
    return isWin_ultra(table, plane, col, row) > 0;
}

// ── NormalPlay ───────────────────────────────────────────────────────────────

GameController::MoveResult GameController::submitMove(int col, int row)
{
    if (_winner.has_value())
        return MoveResult::Illegal;

    ensurePosition();

    const Color      color     = currentColor();
    const CellStatus cellColor = colorToCell(color);
    const char*      colorStr  = (color == Color::Black) ? "Black" : "White";

    if (!_ruleChecker.isLegal(_position->board(), col, row, color))
    {
        Logger::warn("CONTROLLER",
            std::string(colorStr) + " (" + std::to_string(col) + "," + std::to_string(row)
            + ") rejected — illegal move");
        return MoveResult::Illegal;
    }

    // TODO: do it this with BitboardTool
    // Capture mask must be computed BEFORE the bitboard is mutated so both
    // detect_captures and SearchPosition::makeMove see the same pre-move state.
    bitboard19 capturedMask = {};
    detect_captures(_position->board(), col, row, color, capturedMask);

    int newCaptures = popcount_bb(capturedMask);
    if (color == Color::Black) _capturesBlack += newCaptures;
    else                       _capturesWhite += newCaptures;

    // TODO: how owns the bitboard ? 
    // Apply to bitboard (handles captures + Zobrist + turn flip internally).
    _position->makeMove(col, row, cellColor);

    // Sync to the visual 2D board.
    _state.board.placeStoneOfColor(col, row, cellColor);
    bb_for_each_bit(capturedMask, [this](int x, int y) {
        _state.board.clearCell(x, y);
    });

    Logger::debug("CONTROLLER",
        std::string(colorStr) + " → (" + std::to_string(col) + "," + std::to_string(row) + ") ✓");

    // Win by capture: 5 pairs (10 individual stones) taken by the mover.
    int moverCaptures = (color == Color::Black) ? _capturesBlack : _capturesWhite;
    if (moverCaptures >= 10)
    {
        _winner = color;
        Logger::info("CONTROLLER", std::string(colorStr) + " wins by capture!");
        _state.board.switchPlayer();
        return MoveResult::Win;
    }

    // Win by five-in-a-row.
    if (checkWin(_position->board(), color, col, row))
    {
        _winner = color;
        Logger::info("CONTROLLER", std::string(colorStr) + " wins by alignment!");
        _state.board.switchPlayer();
        return MoveResult::Win;
    }

    _state.board.switchPlayer();
    return MoveResult::Ok;
}

// ── Opening phase delegation ──────────────────────────────────────────────────

bool GameController::handleOpeningClick(int col, int row)
{
    CellStatus forced = _state.nextOpeningColor();
    Move m{ col, row, forced };
    return applyOpeningMove(_state, m);
}

void GameController::resolveColorChoice(bool swapped)
{
    _state.resolveColorChoice(swapped);
}

void GameController::continueOpeningPlacement()
{
    _state.continueOpeningPlacement();
}

// ── UI accessors ──────────────────────────────────────────────────────────────

const GameBoard&     GameController::visualBoard()      const { return _state.board; }
GamePhase            GameController::phase()            const { return _state.phase; }
Seat                 GameController::currentActor()     const { return _state.currentActor; }
CellStatus           GameController::nextOpeningColor() const { return _state.nextOpeningColor(); }
OpeningRule          GameController::openingRule()      const { return _state.openingRule; }
int                  GameController::stepIdx()          const { return _state.stepIdx; }
std::optional<Color> GameController::winner()           const { return _winner; }
int                  GameController::captureCount(Color c) const
{
    return (c == Color::Black) ? _capturesBlack : _capturesWhite;
}
