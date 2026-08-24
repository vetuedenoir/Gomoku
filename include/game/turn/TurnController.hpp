#ifndef TURNCONTROLLER_HPP
# define TURNCONTROLLER_HPP

#include "game/controller/IGameController.hpp"
#include "game/contracts/contracts.hpp"
#include "game/turn/WinDetector.hpp"
#include "bitboard/bitboard.hpp"
#include "logger/Logger.hpp"
#include "config/config.hpp"
#include <optional>
#include <string>

template<typename Traits>
class TurnController
{
public:
    // `pending` est le sursis hérité du coup précédent (cf. PendingWin) ; le
    // nouvel état de sursis est renvoyé dans TurnOutcome::pendingWin.
    TurnOutcome<Traits> play(t_BWBoard<Traits>& bb, const Move& move,
                             int capturesBlack, int capturesWhite,
                             const std::optional<PendingWin>& pending = std::nullopt) const;

private:
    static Color colorFromCell(CellStatus cell);
    static const char* colorLabel(const Color color);

    // CaptureResult<Traits> resolveCaptures(t_BWBoard<Traits>& bb, int col, int row,
    //                                       const Color color) const;

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

// template<typename Traits>
// CaptureResult<Traits> TurnController<Traits>::resolveCaptures(t_BWBoard<Traits>& bb, int col,
//                                                               int row, const Color color) const
// {
//     typename Traits::Bitboard capturedMask = {};

//     detect_captures<Traits>(bb, col, row, color, capturedMask);

//     const int newCaptures = popcount_bb_generic<Traits>(capturedMask);

//     return { capturedMask, newCaptures };
// }

template<typename Traits>
void TurnController<Traits>::logMove(const Color color, const Move& move, int newCaptures) const
{
    LOG_DEBUG("TURN",
        std::string(colorLabel(color)) + " → (" + std::to_string(move.col) + ","
        + std::to_string(move.row) + ")"
        + (newCaptures > 0 ? " captures=" + std::to_string(newCaptures) : ""));
    LOG_SUPPRESS(color, move, newCaptures);
}

// Ordre de résolution imposé par la règle de la capture finale :
//   1. dixième capture — elle prime sur tout, y compris sur un cinq en sursis ;
//   2. cinq en sursis de l'adversaire — s'il n'a pas été cassé, son auteur gagne ;
//   3. cinq posé par ce coup — gagnant s'il est imprenable, sinon mis en sursis.
// Un coup peut casser la ligne adverse ET en aligner une : d'où (2) avant (3).
template<typename Traits>
TurnOutcome<Traits> TurnController<Traits>::play(t_BWBoard<Traits>& bb, const Move& move,
                                                 int capturesBlack, int capturesWhite,
                                                 const std::optional<PendingWin>& pending) const
{
    const Color color = colorFromCell(move.forcedColor);

    const CaptureResult<Traits> caps = BitboardTool<Traits>::instance().resolveCaptures(bb, move.col, move.row, color);

    set_bb_generic<Traits>(bitboardForColor<Traits>(bb, color), move.col, move.row);
    apply_captures<Traits>(bb, caps.mask, color);

    logMove(color, move, caps.count);

    const int blackAfter = capturesBlack + (color == Color::Black ? caps.count : 0);
    const int whiteAfter = capturesWhite + (color == Color::White ? caps.count : 0);

    const int moverCaptures    = (color == Color::Black) ? blackAfter : whiteAfter;
    const int defenderCaptures = (color == Color::Black) ? whiteAfter : blackAfter;

    // 1. Victoire par capture.
    if (moverCaptures >= CAPTURES_TO_WIN)
    {
        Logger::info("TURN", std::string(colorLabel(color)) + " wins by capture!");
        return { MoveResult::Win, caps.count, color, caps.mask, std::nullopt };
    }

    // 2. Sort du cinq laissé en sursis par l'adversaire.
    std::optional<PendingWin> nextPending = std::nullopt;

    if (pending.has_value() && pending->owner != color)
    {
        if (pendingFiveSurvives<Traits>(bb, *pending))
        {
            Logger::info("TURN", std::string(colorLabel(pending->owner))
                + " wins by alignment (five survived the capture attempt)!");
            return { MoveResult::Win, caps.count, pending->owner, caps.mask, std::nullopt };
        }
        Logger::info("TURN", std::string(colorLabel(color)) + " broke the pending five");
    }
    else
    {
        nextPending = pending; // sursis du camp au trait : ne se résout pas ici
    }

    // 3. Cinq créé par ce coup.
    const FiveVerdict verdict =
        judgeFiveAfterMove<Traits>(bb, color, move.col, move.row, defenderCaptures);

    if (verdict == FiveVerdict::Won)
    {
        Logger::info("TURN", std::string(colorLabel(color)) + " wins by alignment!");
        return { MoveResult::Win, caps.count, color, caps.mask, std::nullopt };
    }

    // TODO — la nulle (FiveVerdict::Draw) n'est pas encore une issue de partie :
    // MoveResult n'a pas de Draw et GameController ne sait pas la restituer. En
    // attendant, on la traite comme un sursis, ce qui rend le comportement
    // d'avant (le défenseur casse la ligne et gagne par capture). Seul minimax
    // applique la règle complète pour l'instant.
    if (verdict == FiveVerdict::Pending || verdict == FiveVerdict::Draw)
    {
        Logger::info("TURN", std::string(colorLabel(color))
            + " has five in a row — breakable, " + colorLabel(opponentOf(color))
            + " has one move to capture a pair");
        nextPending = PendingWin{ color, move.col, move.row };
    }

    return { MoveResult::Ok, caps.count, std::nullopt, caps.mask, nextPending };
}

using TurnController19 = TurnController<BoardTraits<19>>;
using TurnController15 = TurnController<BoardTraits<15>>;

#endif
