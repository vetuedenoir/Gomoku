#ifndef ACTIVEZONE_HPP
# define ACTIVEZONE_HPP


#include "game/contracts/Move.hpp"
#include "game/GameBoard.hpp"
#include <unordered_set>
#include <cstddef>

struct MoveHash
{
    std::size_t operator()(const Move& c) const noexcept
    {
        return (static_cast<std::size_t>(c.col) << 32)
             ^ static_cast<std::size_t>(c.row);
    }
};

// todo: maybe add this ? 
// stone_count (if you want dynamic radius)
// reference or pointer to GameBoard (read-only)




class ActiveZone
{
    public:
        
        using CandidateSet = std::unordered_set<Move, MoveHash>;
        
        explicit ActiveZone(int radius);
        

        void initialize(const GameBoard& GameBoard);
        
        // void onMovePlayed(const Move& move, const GameBoard& GameBoard);
        
        const CandidateSet& getCandidates() const noexcept;
        
        bool contains(const Move& m) const noexcept;
        
        std::size_t size() const noexcept;
        
        void clear();
        
        void setRadius(int radius) noexcept;
        
        int getRadius() const noexcept;
    
    private:
        CandidateSet    _candidates;
        
        int             _radius;
        
        void addNeighbors(const Move& center, const GameBoard& GameBoard);
};

#endif