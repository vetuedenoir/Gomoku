#ifndef TURNCONTROLLER_HPP
# define TURNCONTROLLER_HPP

#include "game/controller/IGameController.hpp"
#include "game/GameState.hpp"
#include "game/contracts/contracts.hpp"
#include "game/turn/WinDetector.hpp"
#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"
#include <optional>
#include <string>

template<typename Traits>
class TurnController
{
public:
    TurnOutcome play(GameState& state, int& capturesBlack, int& capturesWhite, const Move& move);

private:
    static Color colorFromCell(CellStatus cell);
    static const char* colorLabel(const Color color);

    CaptureResult<Traits> resolveCaptures(t_BWBoard<Traits>& bb, int col, int row, const Color color) const;

    void commitMove(GameState& state, t_BWBoard<Traits>& bb, const Move& move,
                    const typename Traits::Bitboard& capturedMask) const;

    std::optional<Color> checkWin(const t_BWBoard<Traits>& bb, const Color color, int col, int row,
                                  int capturesBlack, int capturesWhite) const;

    void logMove(const Color color, const Move& move, int newCaptures) const;
};

template<typename Traits>
Color TurnController<Traits>::colorFromCell(CellStatus cell)
{
    return (cell == CellStatus::Black) ? Color::Black : Color::White;
}

template<typename Traits>
const char* TurnController<Traits>::colorLabel(const Color color)
{
    return (color == Color::Black) ? "Black" : "White";
}

template<typename Traits>
CaptureResult<Traits> TurnController<Traits>::resolveCaptures(t_BWBoard<Traits>& bb, int col,
                                                              int row, const Color color) const
{
    typename Traits::Bitboard capturedMask = {};
    
    detect_captures<Traits>(bb, col, row, color, capturedMask);

    const int newCaptures = popcount_bb_generic<Traits>(capturedMask);

    return { capturedMask, newCaptures };
}

template<typename Traits>
void TurnController<Traits>::commitMove(GameState& state, t_BWBoard<Traits>& bb, const Move& move,
                                        const typename Traits::Bitboard& capturedMask) const
{
    const Color      color     = colorFromCell(move.forcedColor);
    const CellStatus cellColor = move.forcedColor;

    set_bb_generic<Traits>(bitboardForColor<Traits>(bb, color), move.col, move.row);
    apply_captures<Traits>(bb, capturedMask, color);

    state.board->placeStoneOfColor(move.col, move.row, cellColor);
    bb_for_each_bit<Traits>(capturedMask, [&state](int x, int y) {
        state.board->clearCell(x, y);
    });
}

template<typename Traits>
std::optional<Color> TurnController<Traits>::checkWin(const t_BWBoard<Traits>& bb, const Color color,
                                                      int col, int row, int capturesBlack,
                                                      int capturesWhite) const
{
    const int moverCaptures =
        (color == Color::Black) ? capturesBlack : capturesWhite;

    if (moverCaptures >= CAPTURES_TO_WIN)
    {
        Logger::info("TURN", std::string(colorLabel(color)) + " wins by capture!");
        return color;
    }

    if (isWinAfterMove<Traits>(bb, color, col, row))
    {
        Logger::info("TURN", std::string(colorLabel(color)) + " wins by alignment!");
        return color;
    }

    return std::nullopt;
}

template<typename Traits>
void TurnController<Traits>::logMove(const Color color, const Move& move, int newCaptures) const
{
    Logger::debug("TURN",
        std::string(colorLabel(color)) + " → (" + std::to_string(move.col) + ","
        + std::to_string(move.row) + ")"
        + (newCaptures > 0 ? " captures=" + std::to_string(newCaptures) : ""));
}

template<typename Traits>
TurnOutcome TurnController<Traits>::play(GameState& state, int& capturesBlack, int& capturesWhite,
                                        const Move& move)
{
    const Color color = colorFromCell(move.forcedColor);
    t_BWBoard<Traits> bb = GameBoard_to_bitboard<Traits>(*state.board);

    const CaptureResult<Traits> caps = resolveCaptures(bb, move.col, move.row, color);
    
    commitMove(state, bb, move, caps.mask);
    
    logMove(color, move, caps.count);

    // Checkpoint:  une notion de winner
    const std::optional<Color> winner =
            checkWin(bb, color, move.col, move.row, capturesBlack, capturesWhite);

    if (winner.has_value())
        return { MoveResult::Win, caps.count, winner.value() };

    return { MoveResult::Ok, caps.count, std::nullopt };
}

using TurnController19 = TurnController<BoardTraits<19>>;
using TurnController15 = TurnController<BoardTraits<15>>;

#endif
