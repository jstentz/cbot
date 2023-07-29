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

#include "include/board.h"
#include "include/move_gen.h"
#include "include/openings.h"
#include "include/evaluation.h"
#include "include/tt.h"

#include <stddef.h>
#include <cstdint>
#include <thread>

// new class implementation

/// TODO: construct the transposition table inside of here
class Searcher
{
public:
  using Ptr = std::shared_ptr<Searcher>;
  using ConstPtr = std::shared_ptr<const Searcher>;
  
  Searcher(Board::Ptr board);

  uint64_t perft(int depth);
  uint64_t num_nodes_bulk(int depth);
  uint64_t num_nodes(int depth);
  void find_best_move(); // searches forever 
  void ponder();
  void stop();
  void find_best_move_timed(int time); // searches for time milliseconds
  Move get_best_move();
  void abort_search();

private:
  Board::Ptr m_board;
  MoveGenerator m_move_gen;
  OpeningBook m_opening_book;
  Evaluator m_evaluator;
  TranspositionTable m_tt;

  std::thread m_search_thread;

  Move m_best_move;
  Move m_best_move_this_iteration;
  int m_best_score_this_iteration;
  int m_best_score;

  bool m_abort_search;

  int qsearch(int alpha, int beta);
  int search(int ply_from_room, int depth, int alpha, int beta, bool is_pv = false, bool can_null = false);
};