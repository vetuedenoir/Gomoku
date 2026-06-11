#ifndef CONTRACTS_HPP
#define CONTRACTS_HPP

#include <optional>
#include <string>

enum class Color { Black = 0, White = 1 };
enum class GamePhase { Opening, ColorChoice, Standard };
enum class MoveResult { Illegal, Ok, Win };
enum class OpeningProtocol { Standard, Pro, LongPro, Swap, Swap2 };

struct GameConfig
{
    int             boardSize       = 19;
    Color           playerColor     = Color::Black;
    OpeningProtocol openingProtocol = OpeningProtocol::Standard;
    bool            aiOpponent      = true;
};


enum class CellStatus { Empty, Black, White, AI };

inline CellStatus colorToCell(Color c) noexcept
{
    return (c == Color::Black) ? CellStatus::Black : CellStatus::White;
}

// A move in the game.
// During Opening, forcedColor is set by the opening script;
// During Standard, forcedColor == CellStatus::Empty (use current player).
struct Move {
  int col;
  int row;
  CellStatus forcedColor;

  bool operator==(const Move &other) const noexcept {
    return col == other.col && row == other.row;
  }
};

// Identifies a physical seat at the table.
// Stable for the entire game; independent of colour assignment.
enum class Seat : int { First = 0, Second = 1 };

inline Seat otherSeat(Seat s) {
  return (s == Seat::First) ? Seat::Second : Seat::First;
}

inline std::string seatStr(Seat s) {
  return (s == Seat::First) ? "Seat::First" : "Seat::Second";
}

struct Actor {
  Seat seat;
  Color color;
};

template <typename Traits> struct CaptureResult {
  typename Traits::Bitboard mask;
  int count;
};

template <typename Traits>
struct TurnOutcome {
  MoveResult result;
  int capturesAdded; // for mover's color
  std::optional<Color> winnerByColor;
  typename Traits::Bitboard capturedMask{};
};

#endif