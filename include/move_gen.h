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

#include <vector>
#include <stack>
#include <iostream>
#include <string>

#include "include/move.h"
#include "include/board.h"

/**
 * @brief Given a board state, generates all legal moves in the position.
 * 
 * @param board Current board state
 * @param curr_moves Place to store the moves
 * @param captures_only Whether to generate only captures or all legal moves
 */
void generate_moves(std::vector<move_t> &curr_moves, bool captures_only = false);

/**
 * @brief Given a list of moves, tries to order the moves from best to worse
 * using some heuristics.
 * 
 * @param moves Vector of moves to reorder
 */
void order_moves(std::vector<move_t> &moves, move_t tt_best_move);

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
std::string notation_from_move(move_t move);

/**
 * @brief Given the notation for a move and the current position, returns the
 * move struct representing that move.
 * 
 * @param notation Move
 * @param board Current position
 * @return Move representing the notation
 */
move_t move_from_notation(std::string notation);

/**
 * @brief Builds the bit pattern to represent the move based on the inputs.
 * 
 * @param from Square the piece is coming from
 * @param to Square the piece is moving to
 * @param flags Type of move
 * @return move_t 
 */
move_t construct_move(int from, int to, int flags);

/**
 * @brief Returns the score of a given capture move ignoring all other aspects of the position,
 * except the enemy and friendly pieces attacking a given square.
 * 
 * @param capture 
 * @return int score
 */
int see_capture(move_t capture);

/**
 * @brief Returns true if the input capture move is bad, and false otherwise.
 * 
 * @param capture 
 * @return true 
 * @return false 
 */
bool is_bad_capture(move_t capture);

/**
 * @brief Returns true if the move is a promotion move or if the move is pushing a pawn to the 7th or 2nd
 * rank respectively. Make sure to call this function before making the move on the board.
 * 
 * @param move 
 * @return true 
 * @return false 
 */
bool pawn_promo_or_close_push(move_t move);

/**
 * @brief Returns the move in algebraic notation (eg a1c1).
 * 
 * @param move 
 * @return string 
 */
std::string move_to_long_algebraic(move_t move);

move_t long_algebraic_to_move(std::string notation);


/**
 * @brief Function to sort a list of moves by their algebraic notation.
 * 
 * @param moves 
 */
void sort_by_algebraic_notation(std::vector<move_t> &moves);


// make this take in a pointer to the board, so that the internal
class MoveGenerator
{
public:
  MoveGenerator(Board::Ptr board);



private:
  Board::Ptr m_board;
  // likely need the lookup table here for move generation 
  // create it static so it calls constructor at runtime
  // const static LUT lut;
};