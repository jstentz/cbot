/**
 * @file bitboard.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines basic operations for bitboards
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <cstdint>

#include "include/constants.h"

#define REMOVE_FIRST(a) ((a) = ((a) & ((a)-1)))
#define BIT_FROM_SQ(a) ((bitboard)0x1 << a) // replace all luts.pieces with this

typedef long long unsigned int bitboard;

inline bitboard rotate_right(bitboard b, int s) {return (b >> s) | (b << (64 - s));}

inline bitboard flip_vertical(bitboard b) {
  return  ((b << 56)                               ) |
      ((b << 40) & bitboard(0x00ff000000000000)) |
      ((b << 24) & bitboard(0x0000ff0000000000)) |
      ((b <<  8) & bitboard(0x000000ff00000000)) |
      ((b >>  8) & bitboard(0x00000000ff000000)) |
      ((b >> 24) & bitboard(0x0000000000ff0000)) |
      ((b >> 40) & bitboard(0x000000000000ff00)) |
      ((b >> 56));
}

inline bitboard flip_diag_a1_h8(bitboard b) {
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

inline bitboard rotate_90_clockwise(bitboard b) {
  return flip_vertical(flip_diag_a1_h8(b));
}

inline bitboard rotate_90_anticlockwise(bitboard b) {
  return flip_diag_a1_h8(flip_vertical(b));
}

inline bitboard pseudo_rotate_45_clockwise(bitboard b) {
  const bitboard k1 = bitboard(0xAAAAAAAAAAAAAAAA);
  const bitboard k2 = bitboard(0xCCCCCCCCCCCCCCCC);
  const bitboard k4 = bitboard(0xF0F0F0F0F0F0F0F0);
  
  b ^= k1 & (b ^ rotate_right(b, 8));
  b ^= k2 & (b ^ rotate_right(b, 16));
  b ^= k4 & (b ^ rotate_right(b, 32));
  return b;
}

inline bitboard pseudo_rotate_45_anticlockwise(bitboard b) {
  const bitboard k1 = bitboard(0x5555555555555555);
  const bitboard k2 = bitboard(0x3333333333333333);
  const bitboard k4 = bitboard(0x0F0F0F0F0F0F0F0F);
  
  b ^= k1 & (b ^ rotate_right(b, 8));
  b ^= k2 & (b ^ rotate_right(b, 16));
  b ^= k4 & (b ^ rotate_right(b, 32));
  return b;
}

inline bitboard undo_pseudo_rotate_45_clockwise(bitboard b) {
  for (size_t i = 0; i < 7; i++) {
    b = pseudo_rotate_45_clockwise(b);
  }
  return b;
}

inline bitboard undo_pseudo_rotate_45_anticlockwise(bitboard b) {
  for (size_t i = 0; i < 7; i++) {
    b = pseudo_rotate_45_anticlockwise(b);
  }
  return b;
}

inline bitboard rem_first_bit(bitboard bits) {
  return bits & (bits - 1);
}

inline int pop_count(bitboard b) {
  if(b == 0) return 0;
  const bitboard k1 = (bitboard)0x5555555555555555;
  const bitboard k2 = (bitboard)0x3333333333333333;
  const bitboard k4 = (bitboard)0x0F0F0F0F0F0F0F0F;
  b = b - ((b >> 1) & k1);
  b = (b & k2) + ((b >> 2) & k2);
  b = (b + (b >> 4)) & k4;
  b = (b * ((bitboard)0x0101010101010101)) >> 56;
  return (int) b;
}

inline uint16_t first_set_bit(bitboard bits) {
  return constants::index64[((bits ^ (bits-1)) * constants::debruijn64) >> 58];
}