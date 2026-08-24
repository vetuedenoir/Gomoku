#ifndef IGAMECONTROLLER_HPP
# define IGAMECONTROLLER_HPP

#include "game/contracts/contracts.hpp"
#include "game/board/GameBoard.hpp"
#include <memory>
#include <optional>

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

        virtual std::optional<OpeningDecision> takeOpeningDecision() = 0;

        virtual std::optional<Move> suggestMove() = 0;

        virtual const GameBoard&     visualBoard()      const = 0;
        virtual GamePhase            phase()            const = 0;
        virtual Actor                currentActor()     const = 0;
        virtual CellStatus           nextOpeningColor() const = 0;
        virtual Color                currentColor()     const = 0;
        virtual OpeningProtocol      openingProtocol()  const = 0;
        virtual int                  stepIdx()          const = 0;
        virtual std::optional<Color> getColorFromWinningActor()           const = 0;

        // Cinq aligné qui attend la réponse de l'adversaire : celui-ci a un coup
        // pour casser la ligne par capture, sinon l'auteur gagne (cf. PendingWin).
        virtual std::optional<PendingWin> pendingWin()                    const = 0;

        virtual Actor                playerActor()      const = 0;
        virtual Actor                aiActor()          const = 0;

        virtual bool                 aiOpponent()       const = 0;
        virtual bool                 aiVsAi()           const = 0;

        virtual double               aiMoveLastMs(Color color)    const = 0;
        virtual double               aiMoveAverageMs(Color color) const = 0;

        virtual int                  blackCaptureCount() const = 0;
        virtual int                  whiteCaptureCount() const = 0;

        // Charge un goban déjà préparé et entre en phase Standard (hotseat / démo).
        virtual void seedStandardPosition(const GameBoard& stones, Color toMove) = 0;
};

std::unique_ptr<IGameController> makeGameController(const GameConfig& config);

#endif
