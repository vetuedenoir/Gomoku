#ifndef IGAMECONTROLLER_HPP
# define IGAMECONTROLLER_HPP

#include "game/contracts/GameConfig.hpp"
#include "game/contracts/Color.hpp"
#include "game/contracts/GamePhase.hpp"
#include "game/contracts/Move.hpp"
#include "game/contracts/OpeningProtocol.hpp"
#include "game/contracts/Stone.hpp"
#include "game/board/GameBoard.hpp"
#include "game/board/Seat.hpp"
#include <memory>
#include <optional>
#include <vector>

const int CAPTURES_TO_WIN = 10;
enum class MoveResult { Illegal, Ok, Win };


class IGameController
{
    public:
        virtual ~IGameController() = default;

        virtual MoveResult submitMove(int col, int row) = 0;

        // todo: uniform this name ?
        virtual bool handleOpeningClick(int col, int row) = 0;
        virtual void resolveColorChoice(bool swapped) = 0;
        virtual void continueOpeningPlacement() = 0;

        virtual std::optional<Move> requestAIMove() = 0;

        virtual const GameBoard&     visualBoard()      const = 0;
        virtual GamePhase            phase()            const = 0;
        virtual Seat                 currentActor()     const = 0;
        virtual CellStatus           nextOpeningColor() const = 0;
        virtual Color                currentColor()     const = 0;
        virtual OpeningProtocol      openingProtocol()  const = 0;
        virtual int                  stepIdx()          const = 0;
        virtual std::optional<Color> winner()           const = 0;
        virtual int                  captureCount(Color c) const = 0;
};

std::unique_ptr<IGameController> makeGameController(const GameConfig& config);

#endif
