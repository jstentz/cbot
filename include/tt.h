/**
 * @file tt.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Outlines the datatype and function definitions for transposition table
 * and repetition checking.
 * @version 0.1
 * @date 2022-06-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include "include/hashing.h"
#include "include/move.h"

#include <climits>
#include <unordered_set>
#include <optional>

/// TODO: make this more in the c++ style (using std::allocator?)s
class TranspositionTable
{
  /// TODO: make this an easier number (megabytes) and then round to a power of 2 for them 
  TranspositionTable(size_t entries);
  ~TranspositionTable(); // make this free the pointer to the memory

  enum class Flags
  {
    EXACT,
    ALPHA,
    BETA
  };

  std::optional<int> fetch(uint64_t hash, int depth, int ply_searched, int alpha, int beta, Move& best_move);
  void store(uint64_t hash, int depth, int ply_searched, int flags, int score, Move& best_move);
  void clear();

private:
  struct Entry
  {
    uint64_t key;
    uint16_t depth;
    char flags;
    int score;
    Move best_move;
  };

  Entry* m_table;
  size_t m_entries;

  int correct_stored_mate_score(int score, int ply_searched);
  int correct_retrieved_mate_score(int score, int ply_searched);
}; 