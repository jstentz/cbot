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
#include <climits>

#define EVAL_SIZE 131072 /* 2^17 THIS MUST BE A POWER OF 2 */
#define ENDGAME_MATERIAL 2000
#define LAZY_EVAL_MARGIN 200 /* we won't consider positions that are two pawns different than alpha / beta */
#define ATTACKING_WEIGHT 15
#define MOBILITY_WEIGHT 1

#define EXACT 0
#define ALPHA 1
#define BETA 2
#define FAILED_LOOKUP INT_MIN

extern float game_phase;

typedef struct eval_entry_struct {
  uint64_t key;
  int score;
  char flags;
} eval_entry;

/**
 * @brief Returns a static evaluation of the board state
 * 
 * @param board board to evaluate
 * @return score from the perspective of who's turn it is
 */
int evaluate(int alpha, int beta);

/**
 * @brief Initializes the eval hash table.
 * 
 */
void init_eval_table();

/**
 * @brief Frees the eval hash table.
 * 
 */
void free_eval_table();

/**
 * @brief Reinitializes the eval hash table.
 * 
 */
void clear_eval_table();

extern int eval_hits;

extern int eval_probes;

/**
 * @brief Given a score, returns true if it is a mating score and false otherwise.
 * 
 * @param score 
 * @return true 
 * @return false 
 */
bool is_mate_score(int score);

/**
 * @brief Given a mating score, returns the moves until checkmate
 * 
 * @param mate_score 
 * @return moves until mate 
 */
int moves_until_mate(int mate_score);

// come back to this after doing the transposition table 
// make sure to add functions to allow us to clear the table externally 
class Evaluator
{
public:
  Evaluator(Board::Ptr m_board);
  ~Evaluator() {}

  int evaluate(int alpha, int beta);

private:
  Board::Ptr m_board;
};