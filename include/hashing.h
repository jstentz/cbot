/**
 * @file hashing.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines the necessary types and functions for hashing boards
 * @version 0.1
 * @date 2022-06-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "include/board.h"

/* this struct defines the random constants used to encode any given position */
typedef struct zobrist_table {
  uint64_t table[64][12];
  uint64_t black_to_move;
  uint64_t white_king_side;
  uint64_t black_king_side;
  uint64_t white_queen_side;
  uint64_t black_queen_side;

  uint64_t en_passant_file[8];
} zobrist_table_t;


/**
 * @brief Initializes the zobrist table with random values.
 * 
 * @return zobrist_table_t 
 */
zobrist_table_t init_zobrist();

/**
 * @brief Returns the hash value of the current board state.
 * 
 * @return uint64_t 
 */
uint64_t zobrist_hash();

/**
 * @brief Returns the hash value of ONLY the pieces on the board. Used for 
 * evaluation hash table.
 * 
 * @return uint64_t 
 */
uint64_t hash_pieces();

/**
 * @brief Returns the hash value of ONLY the pawns on the board. Used for
 * pawn structure evaluation hash table.
 * 
 * @return uint64_t 
 */
uint64_t hash_pawns();

/**
 * @brief Returns a random 64-bit number.
 * 
 * @return uint64_t 
 */
uint64_t rand64();


class Hasher
{
public:
  /// @brief initializes the internal table 
  Hasher();
  ~Hasher() {}

  uint64_t hash_board()


};