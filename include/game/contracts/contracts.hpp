#ifndef CONTRACTS_HPP
#define CONTRACTS_HPP

#include <optional>
#include <string>
#include <array>
#include <cstdint>

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
struct Move {
  int col;
  int row;
  CellStatus forcedColor;

  bool operator==(const Move &other) const noexcept {
    return col == other.col && row == other.row;
  }
};

struct t_cell
{
	int_fast16_t x;
	int_fast16_t y;

	bool operator==(const t_cell& other) const
    {
        return x == other.x && y == other.y;
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


template<typename T, std::size_t N>
struct MoveList
{
    std::array<T, N> moves;
    std::size_t count = 0;

    void clear() noexcept
    {
        count = 0;
    }

    void push(const T& m) noexcept
    {
        moves[count++] = m;
    }

    void pop() noexcept
    {
        --count;
    }

    T& top() noexcept
    {
        return moves[count - 1];
    }

    const T& top() const noexcept
    {
        return moves[count - 1];
    }

    T& operator[](std::size_t i) noexcept
    {
        return moves[i];
    }

    const T& operator[](std::size_t i) const noexcept
    {
        return moves[i];
    }

    std::size_t size() const noexcept
    {
        return count;
    }

    bool empty() const noexcept
    {
        return count == 0;
    }
 
    MoveList& operator=(const MoveList& other) noexcept
    {
        if (this != &other)
        {
            count = other.count;
            std::copy(other.moves.begin(), other.moves.begin() + count, moves.begin());
        }
        return *this;
    }


    MoveList(const MoveList& other) noexcept
    {
        count = other.count;
        std::copy(other.moves.begin(), other.moves.begin() + count, moves.begin());
    }

    MoveList() noexcept = default; // à ne pas oublier si tu déclares un constructeur de copie !
    T* begin() noexcept { return moves.data(); }
    T* end()   noexcept { return moves.data() + count; }  // ← count, pas N !
    
    const T* begin() const noexcept { return moves.data(); }
    const T* end()   const noexcept { return moves.data() + count; }
};



// Étage de la chaîne de motifs (five → four → cross → three) sur lequel la
// cotation du coup s'est arrêtée. Les étages sont ordonnés du plus fort au plus
// faible ; seul ThreeOrQuiet laisse encore la place à un check_cross.
enum class ShapeStage : uint8_t
{
	Terminal,     // cinq alignés ou 10e capture
	OpenFour,
	HalfFour,
	BrokenFour,
	Cross,
	ThreeOrQuiet  // open three, ou aucun motif
};

// Un nœud de recherche en instancie plusieurs centaines sur la pile : la
// structure reste volontairement compacte. Les pierres capturées ne sont pas
// stockées mais encodées dans un masque d'une demi-direction par bit (voir
// detect_capture_mask), et ne sont matérialisées que pour les coups joués.
struct EvaluatedMove
{
	t_cell					move;
	int						score;
	bool					isLegal;
	uint8_t					captureMask;
	// Par défaut l'étage le plus bas : un consommateur qui l'ignore doit
	// supposer que tous les motifs restent à vérifier.
	ShapeStage				stage = ShapeStage::ThreeOrQuiet;
};


#endif