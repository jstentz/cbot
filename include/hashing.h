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

#include "board.h"

/* defines the datatype we will use to hash the board states */
typedef unsigned long long int hash_val;

/* this struct defines the random constants used to encode any given position */
typedef struct zobrist_table {
    hash_val table[64][12];
    hash_val black_to_move;
    hash_val white_king_side;
    hash_val black_king_side;
    hash_val white_queen_side;
    hash_val black_queen_side;

    hash_val en_passant_file[8];
} zobrist_table_t;

extern zobrist_table_t zobrist_table;

/**
 * @brief Initializes the zobrist table with random values.
 * 
 * @return zobrist_table_t 
 */
zobrist_table_t init_zobrist();

/**
 * @brief Returns the hash value of the current board state.
 * 
 * @return hash_val 
 */
hash_val zobrist_hash();

/**
 * @brief Returns the hash value of ONLY the pieces on the board. Used for 
 * evaluation hash table.
 * 
 * @return hash_val 
 */
hash_val hash_pieces();

/**
 * @brief Returns the hash value of ONLY the pawns on the board. Used for
 * pawn structure evaluation hash table.
 * 
 * @return hash_val 
 */
hash_val hash_pawns();

/**
 * @brief Returns a random 64-bit number.
 * 
 * @return hash_val 
 */
hash_val rand64();