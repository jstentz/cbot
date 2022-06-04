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

uint64_t num_nodes_bulk(stack<board_t *> *board, size_t depth);

uint64_t num_nodes(stack<board_t *> *board, size_t depth);

uint64_t perft(board_t *board, size_t depth);

int qsearch(stack<board_t *> *board_stack, int alpha, int beta);

int search(stack<board_t *> *board_stack, size_t depth, int alpha, int beta);

move_t find_best_move(board_t *board);