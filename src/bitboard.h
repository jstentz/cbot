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

#define REMOVE_FIRST(a) ((a) = ((a) & ((a)-1)))
#define BIT_FROM_SQ(a) (0x1 << a)

typedef long long unsigned int bitboard;
typedef short unsigned int uint16_t;

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

/**
 * @author Kim Walisch (2012)
 * 
 * @param bits bitboard to scan
 * @precondition bits != 0 
 * @return index (0..63) of least significant one bit
 */
uint16_t first_set_bit(bitboard bits); 