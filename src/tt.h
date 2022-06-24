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

#define FAILED_LOOKUP INT_MIN

#define TABLE_SIZE 262144 // this is 2^18

// I need to learn more about the difference between alpha and beta cuttoffs
// what is a good size for a transposition table


// use a union here
typedef struct tt {
    bool valid;
    hash_val key;
    int depth;
    int flags;
    int score;
    move_t best_move;
} tt_entry;

typedef struct tt_table {
    tt_entry *table;
    move_t best_move;
} tt_t;

typedef unordered_set<hash_val> history_t;

extern history_t game_history;

bool probe_game_history(hash_val h);

extern tt_t TT;

extern size_t transpositions; // number of transpositions
extern size_t num_entries;

void init_tt_table();

void free_tt_table();

int probe_tt_table(hash_val h, int depth, int alpha, int beta);

void store_entry(hash_val key, int depth, int flags, int score, move_t best_move);

void clear_tt_table();