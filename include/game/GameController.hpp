#ifndef GAMECONTROLLER_HPP
# define GAMECONTROLLER_HPP

#include "game/GameState.hpp"
#include "game/contracts/GameConfig.hpp"
#include "game/contracts/Color.hpp"
#include "game/contracts/GamePhase.hpp"
#include "optimization/SearchPosition.hpp"
#include "game/RuleChecker.hpp"

// Bridges the UI layer and the bitboard engine.
//
// Owns both the visual GameBoard (via GameState).
// submitMove() applies a move to both,
// applying Gomoku capture rules and win detection.
// Opening phase is fully delegated to GameState / OpeningScript helpers.

class GameController
{
public:
    enum class MoveResult { Illegal, Ok, Win };

    explicit GameController(const GameConfig& config);

    // Apply a human or AI move during NormalPlay.
    // Validates legality, syncs both board representations, detects win.
    MoveResult submitMove(int col, int row);

    // Opening phase helpers — thin delegation to GameState / OpeningScript.
    bool handleOpeningClick(int col, int row);
    void resolveColorChoice(bool swapped);
    void continueOpeningPlacement();

    // Read-only queries for the UI.
    const GameBoard&     visualBoard()      const;
    GamePhase            phase()            const;
    Seat                 currentActor()     const;
    CellStatus           nextOpeningColor() const;
    Color                currentColor()     const;
    OpeningRule          openingRule()      const;
    int                  stepIdx()          const;
    std::optional<Color> winner()           const;
    int                  captureCount(Color c) const;

private:
    GameState                     _state;
    std::optional<SearchPosition> _position;
    RuleChecker                   _ruleChecker;
    std::optional<Color>          _winner;
    int                           _capturesBlack = 0;
    int                           _capturesWhite = 0;

    void ensurePosition();
    static bool checkWin(const t_BWBoard19& bb, Color color, int col, int row);
};

#endif
