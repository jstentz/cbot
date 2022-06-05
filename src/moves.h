/**
 * @file moves.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines moves and functions to generate and interact with them
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "board.h"

#include <vector>
#include <stack>
#include <iostream>
#include <string>

/**
 * @brief Struct that represents a chess move.
 */
typedef struct move_struct {
    square start;
    square target;
    piece mv_piece;
    piece tar_piece;
    piece promotion_piece;
} move_t;

/**
 * @brief Given a board state, generates all legal moves in the position.
 * 
 * @param board Current board state
 * @param curr_moves Place to store the moves
 * @param captures_only Whether to generate only captures or all legal moves
 */
void generate_moves(board_t *board, vector<move_t> *curr_moves, bool captures_only = false);

/**
 * @brief Given a list of moves, tries to order the moves from best to worse
 * using some heuristics.
 * 
 * @param moves Vector of moves to reorder
 */
void order_moves(vector<move_t> *moves);

/**
 * @brief Takes in a pointer to the current board state, along with a move
 * to be played. Makes a move on a copy of the input board, and returns
 * a pointer to the new board.
 * 
 * @param board 
 * @param move 
 * @return board_t* 
 */
board_t *make_move(board_t *board, move_t *move);

/**
 * @brief Given a pointer to a stack of boards, it will undo the most
 * recent move.
 * 
 * @param boards 
 */
void unmake_move(stack<board_t *> *board_stack);

/**
 * @brief Given a move, a list of all legal moves, and the board state,
 * this function will return the standard chess notation representing that
 * unique move.
 * 
 * @param move The move to be played
 * @param all_moves Vector of all legal moves in the position
 * @param board Current board state
 * @return Standard chess notation for input move. 
 */
string notation_from_move(move_t move, vector<move_t> all_moves, board_t *board);