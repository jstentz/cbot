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

#define TABLE_SIZE 131072 /* 2^17 THIS MUST BE A POWER OF 2 */

typedef unsigned short int uint16_t; 

/**
 * @brief Holds all of the necessary info about an entry in the transposition
 * table.
 * 
 */
typedef struct tt {
    hash_val key;
    uint16_t depth;
    char flags;
    int score;
    move_t best_move;
} tt_entry;

/**
 * @brief Holds the table itself and the best move after searching the table.
 * 
 */
typedef struct tt_table {
    tt_entry *table;
    move_t best_move;
} tt_t;

typedef unordered_set<hash_val> history_t;

extern history_t game_history;

/**
 * @brief Checks the game history to see if this position has been reached before.
 * 
 * @param h 
 * @return true 
 * @return false 
 */
bool probe_game_history(hash_val h);

extern tt_t TT;

extern size_t tt_hits; // number of transpositions
extern size_t tt_probes;
extern size_t checkmates;

/**
 * @brief Initializes the transposition table.
 * 
 */
void init_tt_table();

/**
 * @brief Frees the memory holding the transposition table.
 * 
 */
void free_tt_table();

/**
 * @brief Probes the transposition table. Returns a score or FAILED_LOOKUP if no position is found.
 * Note that the function also stores the best move if applicable. 
 * 
 * @param h 
 * @param depth 
 * @param ply_searched 
 * @param alpha 
 * @param beta 
 * @return int 
 */
int probe_tt_table(hash_val h, int depth, int ply_searched, int alpha, int beta);

/**
 * @brief Stores an entry in the transposition table based on the inputted parameters.
 * 
 * @param key 
 * @param depth 
 * @param ply_searched 
 * @param flags 
 * @param score 
 * @param best_move 
 */
void store_entry(hash_val key, int depth, int ply_searched, int flags, int score, move_t best_move);

/**
 * @brief Clears the transposition table.
 * 
 */
void clear_tt_table();