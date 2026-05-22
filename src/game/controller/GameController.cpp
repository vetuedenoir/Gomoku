#include "game/controller/GameController.hpp"

template class GameController<BoardTraits<19>>;
template class GameController<BoardTraits<15>>;

std::unique_ptr<IGameController> makeGameController(const GameConfig& config)
{
    if (config.boardSize == 19)
        return std::make_unique<GameController<BoardTraits<19>>>(config);
    return std::make_unique<GameController<BoardTraits<15>>>(config);
}
