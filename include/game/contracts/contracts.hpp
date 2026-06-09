#ifndef CONTRACTS_HPP
# define CONTRACTS_HPP


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


enum class CellStatus { Empty, Black, White };

inline CellStatus colorToCell(Color c) noexcept
{
    return (c == Color::Black) ? CellStatus::Black : CellStatus::White;
}

// A move in the game.
// During Opening, forcedColor is set by the opening script;
// During Standard, forcedColor == CellStatus::Empty (use current player).
struct Move
{
    int        col;
    int        row;
    CellStatus forcedColor;

    bool operator==(const Move& other) const noexcept
    {
        return col == other.col && row == other.row;
    }
};


#endif