#ifndef GAMECONTROLLER_HPP
# define GAMECONTROLLER_HPP

#include "game/controller/IGameController.hpp"
#include "game/GameState.hpp"
#include "game/validation/MoveValidator.hpp"
#include "game/turn/TurnController.hpp"
#include "ai/MasterAI.hpp"

template<typename Traits>
class GameController : public IGameController
{
    public:
        explicit GameController(const GameConfig& config);

        MoveResult submitMove(int col, int row) override;

        bool handleOpeningClick(int col, int row) override;
        void resolveColorChoice(bool swapped) override;
        void continueOpeningPlacement() override;

        std::optional<Move> requestAIMove() override;

        const GameBoard&     visualBoard()      const override;
        GamePhase            phase()            const override;
        
        Actor                currentActor()     const override;
        Actor                playerActor()      const override;
        Actor                aiActor()          const override;

        CellStatus           nextOpeningColor() const override;
        Color                currentColor()     const override;
        OpeningProtocol      openingProtocol()  const override;
        int                  stepIdx()          const override;
        std::optional<Color> getColorFromWinningActor() const override;
        int                  captureCount(const Color c) const override;

    private:
        void assignColorsAfterChoice(bool swapped);
        Color colorForSeat(Seat seat) const;
        void syncBoardCurrentColor();
        void syncCurrentActorFromState();
        void beginNormalPlay();
        void switchActor();

        GameState              _state;
        MoveValidator<Traits>  _validator;
        TurnController<Traits> _turnController;
        MasterAI<Traits>       _masterAI;
        std::optional<Color>   _winner;
        int                    _capturesBlack = 0;
        int                    _capturesWhite = 0;

        Actor _playerActor;
        Actor _aiActor;
        Actor _currentActor;
};

#include "game/controller/GameController.inl"

#endif
