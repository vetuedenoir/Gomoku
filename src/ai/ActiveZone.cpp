#include "ai/ActiveZone.hpp"

// All ActiveZone implementation is in the header (template class).
// Explicit instantiations ensure the two board-size variants are compiled
// into this translation unit, keeping link times predictable.

template class ActiveZone<BoardTraits<19>>;
template class ActiveZone<BoardTraits<15>>;
