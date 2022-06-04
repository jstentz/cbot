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

typedef unsigned long long int hash_val;

typedef struct zobrist_table {
    hash_val table[64][12];
    hash_val black_to_move;
} zobrist_table_t;

hash_val zobrist_hash(board_t *board);