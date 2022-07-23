/**
 * @file search.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Provides an interface for the searching with the engine
 * @version 0.1
 * @date 2022-05-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "board.h"
#include "moves.h"

#include <stddef.h>
#include <stack>

typedef unsigned long long int uint64_t;

#define IS_PV 1
#define NO_PV 0

#define CAN_NULL 1
#define NO_NULL 0

extern size_t transpositions;

typedef struct search_result_struct {
    move_t best_move;
    int score;
} search_t;

/**
 * @brief Given a stack of the board history and a depth to search,
 * returns the number of nodes reached in bulk.
 * 
 * @param board Current board state
 * @param depth Depth to search
 * @return Number of nodes 
 */
uint64_t num_nodes_bulk(size_t depth);

/**
 * @brief Given a stack of the board history and a depth to search,
 * returns the number of nodes reached.
 * 
 * @param board Current board state
 * @param depth Depth to search
 * @return Number of nodes 
 */
uint64_t num_nodes(size_t depth);

/**
 * @brief Given a board state and a depth, prints a perft test.
 * 
 * @param board Current board state
 * @param depth Depth to search
 * @return Number of nodes  
 */
uint64_t perft(size_t depth);

/**
 * @brief Given the board history, searches all captures until there are no
 * more captures.
 * 
 * @param board_stack Board history
 * @param alpha Alpha cutoff
 * @param beta Beta cutoff
 * @return Best score
 */
int qsearch(int alpha, int beta);

/**
 * @brief Given the board history and a depth, find the best score at a 
 * given depth.
 * 
 * @param board_stack Board history
 * @param depth Depth to search
 * @param alpha Alpha cutoff
 * @param beta Beta cutoff
 * @return Best score
 */
int search_moves(int ply_from_root, int depth, int alpha, int beta, bool is_pv, bool can_null);

/**
 * @brief Given a board state, returns the best move.
 * 
 * @param board 
 * @return Best move 
 */
move_t find_best_move();

extern size_t positions_searched; // debugging