#ifndef SEAT_HPP
# define SEAT_HPP

#include <string>

// Identifies a physical seat at the table.
// Stable for the entire game; independent of colour assignment.
enum class Seat : int
{
    First  = 0,
    Second = 1
};

inline Seat otherSeat(Seat s)
{
    return (s == Seat::First) ? Seat::Second : Seat::First;
}

inline std::string seatStr(Seat s)
{
    return (s == Seat::First) ? "Seat::First" : "Seat::Second";
}

#endif
