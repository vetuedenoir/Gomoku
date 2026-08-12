#ifndef IGAMECONTROLLER_HPP
# define IGAMECONTROLLER_HPP

#include "game/contracts/contracts.hpp"
#include "game/board/GameBoard.hpp"
#include <memory>
#include <optional>

const int CAPTURES_TO_WIN = 10;

class IGameController
{
    public:
        virtual ~IGameController() = default;

        virtual MoveResult submitMove(int col, int row) = 0;

        // todo: uniform this name ?
        virtual bool submitOpeningMove(int col, int row) = 0;
        virtual void resolveColorChoice(bool swapped) = 0;
        virtual void continueOpeningPlacement() = 0;

        virtual std::optional<Move> requestAIMove() = 0;

        virtual std::optional<Move> suggestMove() = 0;

        virtual const GameBoard&     visualBoard()      const = 0;
        virtual GamePhase            phase()            const = 0;
        virtual Actor                currentActor()     const = 0;
        virtual CellStatus           nextOpeningColor() const = 0;
        virtual Color                currentColor()     const = 0;
        virtual OpeningProtocol      openingProtocol()  const = 0;
        virtual int                  stepIdx()          const = 0;
        virtual std::optional<Color> getColorFromWinningActor()           const = 0;

        virtual Actor                playerActor()      const = 0;
        virtual Actor                aiActor()          const = 0;

        virtual bool                 aiOpponent()       const = 0;

        virtual double               aiMoveLastMs()     const = 0;
        virtual double               aiMoveAverageMs()  const = 0;

        virtual int                  blackCaptureCount() const = 0;
        virtual int                  whiteCaptureCount() const = 0;
};

std::unique_ptr<IGameController> makeGameController(const GameConfig& config);

#endif
