#ifndef MASTER_AI_HPP
# define MASTER_AI_HPP

#include "game/board/GameBoard.hpp"
#include "game/contracts/GamePhase.hpp"
#include "game/GameState.hpp"
#include "game/contracts/Move.hpp"
#include "bitboard/bitboard.hpp"
#include <optional>
#include <variant>

using t_BWBoard_variant = std::variant<t_BWBoard19, t_BWBoard15>;

class MasterIA
{
    public:
        virtual ~MasterIA() = default;
};

#endif
