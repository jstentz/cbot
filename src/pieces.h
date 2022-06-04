/**
 * @file pieces.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines representation for pieces and interacting with them
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <stddef.h>

typedef short unsigned int piece;

#define WHITE (piece)0x0
#define BLACK (piece)0x1
#define PAWN (piece)0x2
#define KNIGHT (piece)0x4
#define BISHOP (piece)0x6
#define ROOK (piece)0x8
#define QUEEN (piece)0xA
#define KING (piece)0xC
#define EMPTY (piece)0x0

/**
 * @brief Given a piece, it will return the index associated with that piece.
 * Each piece is mapped to an index 0 - 11 inclusive.
 * 
 * @param pc piece
 * @return index
 */
size_t index_from_piece(piece pc);

/**
 * @brief Given a piece, returns true iff it is a sliding piece.
 * 
 * @param pc 
 * @return true if sliding piece
 * @return false if not
 */
bool is_sliding_piece(piece pc);