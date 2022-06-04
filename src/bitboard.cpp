#include <stdlib.h>
#include <stddef.h>
#include "bitboard.h"

bitboard flip_vertical(bitboard b) {
    return  ((b << 56)                               ) |
            ((b << 40) & bitboard(0x00ff000000000000)) |
            ((b << 24) & bitboard(0x0000ff0000000000)) |
            ((b <<  8) & bitboard(0x000000ff00000000)) |
            ((b >>  8) & bitboard(0x00000000ff000000)) |
            ((b >> 24) & bitboard(0x0000000000ff0000)) |
            ((b >> 40) & bitboard(0x000000000000ff00)) |
            ((b >> 56));
}

bitboard flip_diag_a1_h8(bitboard b) {
    bitboard t;
    const bitboard k1 = bitboard(0x5500550055005500);
    const bitboard k2 = bitboard(0x3333000033330000);
    const bitboard k4 = bitboard(0x0F0F0F0F00000000);
    t = k4 & (b ^ (b << 28));
    b ^= t ^ (t >> 28);
    t = k2 & (b ^ (b << 14));
    b ^= t ^ (t >> 14);
    t = k1 & (b ^ (b << 7));
    b ^= t ^ (t >> 7);
    return b;
}

bitboard rotate_90_clockwise(bitboard b) {
    return flip_vertical(flip_diag_a1_h8(b));
}

bitboard rotate_90_anticlockwise(bitboard b) {
    return flip_diag_a1_h8(flip_vertical(b));
}

bitboard rotate_right(bitboard b, int s) {return (b >> s) | (b << (64 - s));}

bitboard pseudo_rotate_45_clockwise(bitboard b) {
    const bitboard k1 = bitboard(0xAAAAAAAAAAAAAAAA);
    const bitboard k2 = bitboard(0xCCCCCCCCCCCCCCCC);
    const bitboard k4 = bitboard(0xF0F0F0F0F0F0F0F0);
    
    b ^= k1 & (b ^ rotate_right(b, 8));
    b ^= k2 & (b ^ rotate_right(b, 16));
    b ^= k4 & (b ^ rotate_right(b, 32));
    return b;
}

bitboard pseudo_rotate_45_anticlockwise(bitboard b) {
    const bitboard k1 = bitboard(0x5555555555555555);
    const bitboard k2 = bitboard(0x3333333333333333);
    const bitboard k4 = bitboard(0x0F0F0F0F0F0F0F0F);
    
    b ^= k1 & (b ^ rotate_right(b, 8));
    b ^= k2 & (b ^ rotate_right(b, 16));
    b ^= k4 & (b ^ rotate_right(b, 32));
    return b;
}

bitboard undo_pseudo_rotate_45_clockwise(bitboard b) {
    for (size_t i = 0; i < 7; i++) {
        b = pseudo_rotate_45_clockwise(b);
    }
    return b;
}

bitboard undo_pseudo_rotate_45_anticlockwise(bitboard b) {
    for (size_t i = 0; i < 7; i++) {
        b = pseudo_rotate_45_anticlockwise(b);
    }
    return b;
}

const int index64[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

uint16_t first_set_bit(bitboard bits) {
    const bitboard debruijn64 = 0x03f79d71b4cb0a89;
    return index64[((bits ^ (bits-1)) * debruijn64) >> 58];
}

bitboard rem_first_bit(bitboard bits) {
    return bits & (bits - 1);
}