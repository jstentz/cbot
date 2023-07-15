/**
 * @file evaluation.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines evaluation constants and board evaluation functions
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "include/board.h"
#include "include/hashing.h"
#include "include/tt.h"
#include <climits>

class Evaluator
{
public:
  Evaluator(Board::Ptr m_board);
  ~Evaluator() {}

  int evaluate(int alpha, int beta);

private:
  Board::Ptr m_board;

  bool sufficient_checkmating_material();
  void calculate_game_phase();
  int mop_up_eval(bool white_winning_side);
  void evaluate_pawns();
  int evaluate_knights(bitboard white_king_squares, bitboard black_king_squares);
  int evaluate_bishops(bitboard white_king_squares, bitboard black_king_squares);
  int evaluate_rooks(bitboard white_king_squares, bitboard black_king_squares);
  int evaluate_queens(bitboard white_king_squares, bitboard black_king_squares);


  const int ENDGAME_MATERIAL = 2000;
  const int LAZY_EVAL_MARGIN = 200;
  const int ATTACKING_WEIGHT = 15;
  const int MOBILITY_WEIGHT = 1;
};