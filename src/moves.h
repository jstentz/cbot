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

// defines the values in the flag of the move
#define QUIET_MOVE            0x0
#define DOUBLE_PUSH           0x1
#define KING_SIDE_CASTLE      0x2
#define QUEEN_SIDE_CASTLE     0x3
#define NORMAL_CAPTURE        0x4
#define EN_PASSANT_CAPTURE    0x5
#define KNIGHT_PROMO          0x8
#define BISHOP_PROMO          0x9
#define ROOK_PROMO            0xA
#define QUEEN_PROMO           0xB
#define KNIGHT_PROMO_CAPTURE  0xC
#define BISHOP_PROMO_CAPTURE  0xD
#define ROOK_PROMO_CAPTURE    0xE
#define QUEEN_PROMO_CAPTURE   0xF

#define FROM(move) (move & 0x3F)
#define TO(move) ((move >> 6) & 0x3F)
#define FLAGS(move) ((move >> 12) & 0xF)
#define IS_CAPTURE(move) (move & 0x4000)
#define IS_PROMO(move) (move & 0x8000)

#define ADD_SCORE_TO_MOVE(move, score) ((score << 16) | move)
#define SCORE(move) (move >> 16)

#define NO_SCORE(move) (move & 0x0000FFFF)

#define NO_MOVE ((move_t)0x0)

// get rid of the square enum its kinda dumb

/**
 * @brief Representation for a move. The first 16 bits hold the from square, 
 * the to square, and the move type. This is signed so the higher order bits can
 * give us moves with a negative score, therefore easier sorting.
 * 
 */
typedef signed int move_t;

/**
 * @brief Given a board state, generates all legal moves in the position.
 * 
 * @param board Current board state
 * @param curr_moves Place to store the moves
 * @param captures_only Whether to generate only captures or all legal moves
 */
void generate_moves(vector<move_t> *curr_moves, bool captures_only = false);

/**
 * @brief Given a list of moves, tries to order the moves from best to worse
 * using some heuristics.
 * 
 * @param moves Vector of moves to reorder
 */
void order_moves(vector<move_t> *moves, move_t tt_best_move);

/**
 * @brief Takes in a pointer to the current board state, along with a move
 * to be played. Makes a move on a copy of the input board, and returns
 * a pointer to the new board.
 * 
 * @param board 
 * @param move 
 * @return board_t* 
 */
void make_move(move_t move);

/**
 * @brief Given a pointer to a stack of boards, it will undo the most
 * recent move.
 * 
 * @param boards 
 */
void unmake_move(move_t move);

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
string notation_from_move(move_t move);

/**
 * @brief Given the notation for a move and the current position, returns the
 * move struct representing that move.
 * 
 * @param notation Move
 * @param board Current position
 * @return Move representing the notation
 */
move_t move_from_notation(string notation);

move_t construct_move(int from, int to, int flags);

int see_capture(move_t capture);

string algebraic_notation(move_t move);

void sort_by_algebraic_notation(vector<move_t> *moves);