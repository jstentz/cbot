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

/**
 * @brief Rotate the bitboard 90 degrees clockwise.
 * 
 * @param b bitboard
 * @return Rotated bitboard 
 */
bitboard rotate_90_clockwise(bitboard b);

/**
 * @brief Rotate the bitboard 90 degrees anticlockwise.
 * 
 * @param b bitboard
 * @return Rotated bitboard 
 */
bitboard rotate_90_anticlockwise(bitboard b);

/**
 * @brief Rotate the bitboard 45 degrees clockwise.
 * 
 * @param b bitboard
 * @return Rotated bitboard 
 */
bitboard pseudo_rotate_45_clockwise(bitboard b);

/**
 * @brief Rotate the bitboard 45 degrees anticlockwise.
 * 
 * @param b bitboard
 * @return Rotated bitboard 
 */
bitboard pseudo_rotate_45_anticlockwise(bitboard b);

/**
 * @brief Undo the 45 degree clockwise rotation.
 * 
 * @param b bitboard
 * @return Unrotated bitboard 
 */
bitboard undo_pseudo_rotate_45_clockwise(bitboard b);

/**
 * @brief Undo the 45 degree anticlockwise rotation.
 * 
 * @param b bitboard
 * @return Unrotated bitboard
 */
bitboard undo_pseudo_rotate_45_anticlockwise(bitboard b);

inline uint16_t first_set_bit(bitboard bits) {
  return constants::index64[((bits ^ (bits-1)) * constants::debruijn64) >> 58];
}

/**
 * @brief Given a bitboard, return the number of 1s in the bit pattern.
 * 
 * @param b bitboard
 * @return int 
 */
int pop_count(bitboard b);