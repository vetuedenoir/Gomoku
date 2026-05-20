#ifndef TURNCONTROLLER_HPP
# define TURNCONTROLLER_HPP

#include "game/controller/IGameController.hpp"
#include "game/GameState.hpp"
#include "game/contracts/Move.hpp"
#include "game/turn/WinDetector.hpp"
#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"
#include <optional>
#include <string>

struct PlayResult
{
    MoveResult           result;
    std::optional<Color> winner;
};

template<typename Traits>
struct CaptureResult
{
    typename Traits::Bitboard mask{};
    int                       count = 0;
};

template<typename Traits>
class TurnController
{
public:
    PlayResult play(GameState& state, int& capturesBlack, int& capturesWhite, const Move& move);

private:
    static Color colorFromCell(CellStatus cell);
    static const char* colorLabel(Color color);

    t_BWBoard<Traits> boardFromState(const GameState& state) const;

    CaptureResult<Traits> resolveCaptures(t_BWBoard<Traits>& bb, int col, int row, Color color,
                                          int& capturesBlack, int& capturesWhite) const;

    void commitMove(GameState& state, t_BWBoard<Traits>& bb, const Move& move,
                    const typename Traits::Bitboard& capturedMask) const;

    std::optional<Color> checkWin(const t_BWBoard<Traits>& bb, Color color, int col, int row,
                                  int capturesBlack, int capturesWhite) const;

    void finishTurn(GameState& state) const;
    void logMove(Color color, const Move& move, int newCaptures) const;
};

template<typename Traits>
Color TurnController<Traits>::colorFromCell(CellStatus cell)
{
    return (cell == CellStatus::Black) ? Color::Black : Color::White;
}

template<typename Traits>
const char* TurnController<Traits>::colorLabel(Color color)
{
    return (color == Color::Black) ? "Black" : "White";
}

template<typename Traits>
t_BWBoard<Traits> TurnController<Traits>::boardFromState(const GameState& state) const
{
    return GameBoard_to_bitboard<Traits>(*state.board);
}

template<typename Traits>
CaptureResult<Traits> TurnController<Traits>::resolveCaptures(t_BWBoard<Traits>& bb, int col,
                                                              int row, Color color,
                                                              int& capturesBlack,
                                                              int& capturesWhite) const
{
    typename Traits::Bitboard capturedMask = {};
    detect_captures<Traits>(bb, col, row, color, capturedMask);

    const int newCaptures = popcount_bb_generic<Traits>(capturedMask);
    if (color == Color::Black)
        capturesBlack += newCaptures;
    else
        capturesWhite += newCaptures;

    return { capturedMask, newCaptures };
}

template<typename Traits>
void TurnController<Traits>::commitMove(GameState& state, t_BWBoard<Traits>& bb, const Move& move,
                                        const typename Traits::Bitboard& capturedMask) const
{
    const Color      color     = colorFromCell(move.forcedColor);
    const CellStatus cellColor = move.forcedColor;

    set_bb_generic<Traits>(bitboardForColor(bb, color), move.col, move.row);
    apply_captures<Traits>(bb, capturedMask, color);

    state.board->placeStoneOfColor(move.col, move.row, cellColor);
    bb_for_each_bit<Traits>(capturedMask, [&state](int x, int y) {
        state.board->clearCell(x, y);
    });
}

template<typename Traits>
std::optional<Color> TurnController<Traits>::checkWin(const t_BWBoard<Traits>& bb, Color color,
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
void TurnController<Traits>::finishTurn(GameState& state) const
{
    state.board->switchPlayer();
}

template<typename Traits>
void TurnController<Traits>::logMove(Color color, const Move& move, int newCaptures) const
{
    Logger::debug("TURN",
        std::string(colorLabel(color)) + " → (" + std::to_string(move.col) + ","
        + std::to_string(move.row) + ")"
        + (newCaptures > 0 ? " captures=" + std::to_string(newCaptures) : ""));
}

template<typename Traits>
PlayResult TurnController<Traits>::play(GameState& state, int& capturesBlack, int& capturesWhite,
                                        const Move& move)
{
    const Color color = colorFromCell(move.forcedColor);
    t_BWBoard<Traits> bb = boardFromState(state);

    const CaptureResult<Traits> caps = resolveCaptures(bb, move.col, move.row, color, capturesBlack, capturesWhite);
    commitMove(state, bb, move, caps.mask);
    logMove(color, move, caps.count);

    if (const std::optional<Color> winner =
            checkWin(bb, color, move.col, move.row, capturesBlack, capturesWhite))
    {
        finishTurn(state);
        return { MoveResult::Win, winner };
    }

    finishTurn(state);
    return { MoveResult::Ok, std::nullopt };
}

using TurnController19 = TurnController<BoardTraits<19>>;
using TurnController15 = TurnController<BoardTraits<15>>;

#endif
