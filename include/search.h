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

#include <stddef.h>
#include <stack>
#include <cstdint>

// new class implementation

/// TODO: construct the transposition table inside of here
class Searcher
{
public:
  Searcher(Board::Ptr board);

  uint64_t perft(int depth);
  uint64_t num_nodes_bulk(int depth);
  uint64_t num_nodes(int depth);
  Move find_best_move(int search_time);

  /// TODO: add more states to be able to interrupt it at any time
  enum class Status
  {
    WORKING,
    ABORT,
    DONE
  };


private:
  Board::Ptr m_board;
  MoveGenerator m_move_gen;

  Move m_best_move;
  int m_score;
  Status m_status;

  bool m_abort_search;
  bool m_search_complete;

  int qsearch(int alpha, int beta);
  int search(int ply_from_room, int depth, int alpha, int beta, bool is_pv = false, bool can_null = false);
};