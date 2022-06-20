/**
 * @file tt.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Outlines the datatype and function definitions for transposition table
 * and repetition checking.
 * @version 0.1
 * @date 2022-06-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include "hashing.h"
#include "moves.h"

#include <unordered_set>

#define EXACT 0
#define ALPHA 1
#define BETA 2

// I need to learn more about the difference between alpha and beta cuttoffs
// what is a good size for a transposition table

typedef struct tt {
    hash_val key;
    int depth;
    int flags;
    int score;
    move_t best_move;
} tt_entry;

typedef unordered_set<hash_val> history_t;

extern history_t game_history;
