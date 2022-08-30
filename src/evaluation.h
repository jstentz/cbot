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

#include "board.h"

#define EVAL_SIZE 131072 /* 2^17 THIS MUST BE A POWER OF 2 */
#define ENDGAME_MATERIAL 2000
#define FAILED_LOOKUP INT_MIN

const int piece_values[10] = {100, // white pawn
                             -100, // black pawn
                              320, // white knight
                             -320, // black knight
                              330, // white bishop
                             -330, // black bishop
                              500, // white rook
                             -500, // black rook
                              900, // white queen
                             -900};// black queen

const int white_pawns_score[64] = 
{
    0,   0,   0,   0,   0,   0,   0,  0,
    5,  10,  10, -20, -20,  10,  10,  5,
    5,  -5, -10,   0,   0, -10,  -5,  5,
    0,   0,   0,  20,  20,   0,   0,  0,
    5,   5,  10,  25,  25,  10,   5,  5,
   10,  10,  20,  30,  30,  20,  10, 10,
   50,  50,  50,  50,  50,  50,  50, 50,
    0,   0,   0,   0,   0,   0,   0,  0
};

const int black_pawns_score[64] = 
{
    0,   0,   0,   0,   0,   0,   0,  0,
  -50, -50, -50, -50, -50, -50, -50,-50,
   -10,  -10,  -20,  -30,  -30,  -20,  -10, -10,
    -5,   -5,  -10,  -25,  -25,  -10,   -5,  -5,
    0,   0,   0,  -20,  -20,   0,   0,  0,
    -5,  5, 10,   0,   0, 10,  5,  -5,
    -5,  -10,  -10, 20, 20,  -10,  -10,  -5,
    0,   0,   0,   0,   0,   0,   0,  0
};

const int white_knights_score[64] = 
{
  -50, -40, -30, -30, -30, -30, -40, -50,
  -40, -20,   0,   5,   5,   0, -20, -40,
  -30,   5,  10,  15,  15,  10,   5, -30,
  -30,   0,  15,  20,  20,  15,   0, -30,
  -30,   5,  15,  20,  20,  15,   5, -30,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -40, -20,   0,   0,   0,   0, -20, -40,
  -50, -40, -30, -30, -30, -30, -40, -50
};

const int black_knights_score[64] = 
{
  50, 40, 30, 30, 30,  30, 40, 50,
  40, 20,   0,   0,   0,   0, 20, 40,
  30,   0,  -10,  -15,  -15,  -10,   0, 30,
  30,   -5,  -15,  -20,  -20,  -15,   -5, 30,
  30,   0,  -15,  -20,  -20,  -15,   0, 30,
  30,   -5,  -10,  -15,  -15,  -10,   -5, 30,
  40, 20,   0,   -5,   -5,   0, 20, 40,
  50, 40, 30, 30, 30, 30, 40, 50
};

const int white_bishops_score[64] = 
{
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,   5,   0,   0,   0,   0,   5, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   0,  10,  10,  10,  10,   0, -10,
  -10,   5,   5,  10,  10,   5,   5, -10,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10, -10, -10, -10, -10, -20
};

const int black_bishops_score[64] = 
{
  20, 10, 10, 10, 10, 10, 10, 20,
  10,   0,   0,   0,   0,   0,   0, 10,
  10,   0,   -5,  -10,  -10,   -5,   0, 10,
  10,   -5,   -5,  -10,  -10,   -5,   -5, 10,
  10,   0,  -10,  -10,  -10,  -10,   0, 10,
  10,  -10,  -10,  -10,  -10,  -10,  -10, 10,
  10,   -5,   0,   0,   0,   0,   -5, 10,
  20, 10, 10, 10, 10, 10, 10, 20
};

const int white_rooks_score[64] = 
{
   0,    0,   0,   5,   5,   0,   0,   0,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
   5,   10,  10,  10,  10,  10,  10,   5,
   0,    0,   0,   0,   0,   0,   0,   0
};

const int black_rooks_score[64] = 
{
   0,    0,   0,   0,   0,   0,   0,   0,
   -5,   -10,  -10,  -10,  -10,  -10,  -10,   -5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
   0,    0,   0,   -5,   -5,   0,   0,   0
};

const int white_queens_score[64] = 
{
 -20,  -10, -10,  -5,  -5, -10, -10,  -20,
 -10,    0,   5,   0,   0,   0,   0, -10,
 -10,    5,   5,   5,   5,   5,   0, -10,
   0,    0,   5,   5,   5,   5,   0,  -5,
  -5,    0,   5,   5,   5,   5,   0,  -5,
 -10,    0,   5,   5,   5,   5,   0, -10,
 -10,    0,   0,   0,   0,   0,   0, -10,
 -20,  -10, -10,  -5,  -5, -10, -10,  -20
};

const int black_queens_score[64] = 
{
 20,  10, 10,  5,  5, 10, 10,  20,
 10,    0,   0,   0,   0,   0,   0, 10,
 10,    0,   -5,   -5,   -5,   -5,   0, 10,
  5,    0,   -5,   -5,   -5,   -5,   0,  5,
   0,    0,   -5,   -5,   -5,  -5,   0,  5,
 10,    -5,   -5,   -5,   -5,   -5,   0, 10,
 10,    0,   -5,   0,   0,   0,   0, 10,
 20,  10, 10,  5,  5, 10, 10, 20
};

const int white_king_middlegame_score[64] = 
{
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};

const int black_king_middlegame_score[64] = 
{
    30,40,40,50,50,40,40,30,
    30,40,40,50,50,40,40,30,
    30,40,40,50,50,40,40,30,
    30,40,40,50,50,40,40,30,
    20,30,30,40,40,30,30,20,
    10,20,20,20,20,20,20,10,
     -20, -20,  0,  0,  0,  0, -20, -20,
     -20, -30, -10,  0,  0, -10, -30, -20
};

const int white_king_endgame_score[64] = 
{
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -50,-40,-30,-20,-20,-30,-40,-50
};

const int black_king_endgame_score[64] = 
{
    50,40,30,20,20,30,40,50,
    30,20,10,  0,  0,10,20,30,
    30,10, -20, -30, -30, -20,10,30,
    30,10, -30, -40, -40, -30,10,30,
    30,10, -30, -40, -40, -30,10,30,
    30,10, -20, -30, -30, -20,10,30,
    30,30,  0,  0,  0,  0,30,30,
    50,30,30,30,30,30,30,50
};

/**
 * @brief Contains an array of piece-location scores from white's perspective.
 * 
 */
static const int *piece_scores[14] = 
{
    white_pawns_score,
    black_pawns_score,
    white_knights_score,
    black_knights_score,
    white_bishops_score,
    black_bishops_score,
    white_rooks_score,
    black_rooks_score,
    white_queens_score,
    black_queens_score,
    white_king_middlegame_score,
    black_king_middlegame_score,
    white_king_endgame_score,
    black_king_endgame_score
};

extern float game_phase;

typedef struct eval_entry_struct {
    hash_val key;
    int score;
} eval_entry;

/**
 * @brief Returns a static evaluation of the board state
 * 
 * @param board board to evaluate
 * @return score from the perspective of who's turn it is
 */
int evaluate();

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