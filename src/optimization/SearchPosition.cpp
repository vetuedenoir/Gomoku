#include "optimization/SearchPosition.hpp"

// All SearchPosition implementation is in the header (template class).
// Explicit instantiations ensure the two board-size variants are compiled
// into this translation unit, keeping link times predictable.

template class SearchPosition<BoardTraits<19>>;
template class SearchPosition<BoardTraits<15>>;
