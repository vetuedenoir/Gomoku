#include "ai/MoveGenerator.hpp"

// All MoveGenerator implementation is in the header (template class).
// Explicit instantiations ensure the two board-size variants are compiled
// into this translation unit.

template class MoveGenerator<BoardTraits<19>>;
template class MoveGenerator<BoardTraits<15>>;
