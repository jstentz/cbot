#include "include/tt.h"
#include "include/hashing.h"
#include "include/move.h"
#include "include/utils.h"

#include <unordered_set>
#include <cstdlib>
#include <memory.h>
#include <iostream>

int TranspositionTable::correct_retrieved_mate_score(int score, int ply_searched) 
{
  if(utils::is_mate_score(score)) 
  {
    int sign = (score >= 0) ? 1 : -1;
    return (score * sign - ply_searched) * sign; /* correct it by adding onto it how far in the search we are from the root */
  }
  return score;
}

int TranspositionTable::correct_stored_mate_score(int score, int ply_searched) 
{
  if(utils::is_mate_score(score)) 
  {
    int sign = (score >= 0) ? 1 : -1;
    return (score * sign + ply_searched) * sign; /* correct it by adding onto it how far in the search we are from the root */
  }
  return score;
}

TranspositionTable::TranspositionTable(size_t entries) : m_entries(entries)
{
  m_table = (Entry *)calloc(sizeof(Entry), entries);
}

TranspositionTable::~TranspositionTable()
{
  free(m_table);
}

void TranspositionTable::clear()
{
  memset(m_table, 0, sizeof(Entry) * m_entries);
}

std::optional<int> TranspositionTable::fetch_score(uint64_t hash, int depth, int ply_searched, int alpha, int beta)
{
  Entry entry = m_table[hash & (m_entries - 1)];
  if (entry.key == hash && entry.depth >= depth) 
  {
    int corrected_score = correct_retrieved_mate_score(entry.score, ply_searched);
    if (entry.flags == EXACT)
      return std::make_optional(corrected_score);
    if (entry.flags == ALPHA && corrected_score <= alpha)
      return std::make_optional(alpha);
    if (entry.flags == BETA && corrected_score >= beta)
      return std::make_optional(beta);
  }
  return std::nullopt;
}

Move TranspositionTable::fetch_best_move(uint64_t hash)
{
  Entry entry = m_table[hash & (m_entries - 1)];
  if (entry.key == hash)
  {
    return entry.best_move;
  }
  return Move::NO_MOVE;
}

std::optional<int> TranspositionTable::fetch_score(uint64_t hash, int alpha, int beta)
{
  Entry entry = m_table[hash & (m_entries - 1)];
  if(entry.key == hash) 
  {
    int score = entry.score;
    if(entry.flags == EXACT)
      return std::make_optional(score);
    if(entry.flags == ALPHA && score <= alpha)
      return std::make_optional(alpha);
    if(entry.flags == BETA && score >= beta)
      return std::make_optional(beta);
  }
  return std::nullopt;
}

void TranspositionTable::store(uint64_t hash, int depth, int ply_searched, Flags flags, int score, Move best_move)
{
  int corrected_score = correct_stored_mate_score(score, ply_searched);
  Entry* entry = &m_table[hash & (m_entries - 1)];
  entry->key ? m_overwrites++ : m_filled_entries++;
  entry->key = hash;
  entry->depth = depth;
  entry->flags = flags;
  entry->score = corrected_score;
  entry->best_move = best_move;
}

void TranspositionTable::store(uint64_t hash, Flags flags, int score)
{
  Entry* entry = &m_table[hash & (m_entries - 1)];
  entry->key = hash;
  entry->score = score;
  entry->flags = flags;
}

double TranspositionTable::get_occupancy()
{
  return (double) m_filled_entries / (double) m_entries;
}

