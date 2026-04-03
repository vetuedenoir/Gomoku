#ifndef BITBOARD_HPP
# define BITBOARD_HPP

// ── bitboard ─────────────────────────────────────────────────────────────────

struct Bitboard {
    uint64_t b[6] = {};

};

// il y a un padding de 1 bit entre les lignes, donc on peut faire 20x20 = 400 cases 
// avec 6 uint64_t (384 bits) + 16 bits de padding
inline int index(int x, int y) {
    return y * 20 + x;
}

inline void set(Bitboard &bb, int x, int y)
{
    int idx = index(x, y);
    bb.b[idx / 64] |= (1ULL << (idx % 64));
}

inline bool get(const Bitboard& bb, int i) {
    return (bb.b[i >> 6] >> (i & 63)) & 1ULL;
}

#endif // BITBOARD_HPP