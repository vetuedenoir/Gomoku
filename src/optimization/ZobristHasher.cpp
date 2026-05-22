#include "optimization/ZobristHasher.hpp"

// All ZobristHasher implementation is in the header (template class).
// This translation unit triggers instantiation of the two board-size variants
// so the static member definitions are emitted exactly once.

template class ZobristHasher<BoardTraits<19>>;
template class ZobristHasher<BoardTraits<15>>;
