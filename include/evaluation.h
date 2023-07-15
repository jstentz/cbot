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

  void clear_eval_table();

private:
  Board::Ptr m_board;
  LookUpTable lut; // would like to not have to repeat this in the future
  TranspositionTable m_table;

  bool sufficient_checkmating_material();
  float calculate_game_phase();
  int mop_up_eval(bool white_winning);
  int evaluate_pawns();
  int evaluate_knights(bitboard white_king_squares, bitboard black_king_squares);
  int evaluate_bishops(bitboard white_king_squares, bitboard black_king_squares);
  int evaluate_rooks(bitboard white_king_squares, bitboard black_king_squares);
  int evaluate_queens(bitboard white_king_squares, bitboard black_king_squares);
};