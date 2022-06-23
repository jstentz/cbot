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
uint64_t num_nodes_bulk(stack<board_t> *board_stack, size_t depth);

/**
 * @brief Given a stack of the board history and a depth to search,
 * returns the number of nodes reached.
 * 
 * @param board Current board state
 * @param depth Depth to search
 * @return Number of nodes 
 */
uint64_t num_nodes(stack<board_t> *board_stack, size_t depth);

/**
 * @brief Given a board state and a depth, prints a perft test.
 * 
 * @param board Current board state
 * @param depth Depth to search
 * @return Number of nodes  
 */
uint64_t perft(board_t *board, size_t depth);

/**
 * @brief Given the board history, searches all captures until there are no
 * more captures.
 * 
 * @param board_stack Board history
 * @param alpha Alpha cutoff
 * @param beta Beta cutoff
 * @return Best score
 */
int qsearch(stack<board_t> *board_stack, int alpha, int beta);

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
int search(stack<board_t> *board_stack, size_t depth, int alpha, int beta);

/**
 * @brief Given a board state, returns the best move.
 * 
 * @param board 
 * @return Best move 
 */
move_t find_best_move(board_t board);

move_t idss(board_t board);

extern size_t positions_searched; // debugging