#include "include/tt.h"
#include "include/hashing.h"
#include "include/move.h"
#include "include/evaluation.h"

#include <unordered_set>
#include <cstdlib>

tt_t TT;

size_t tt_hits = 0;

size_t tt_probes = 0;

size_t checkmates = 0;

void init_tt_table() {
  TT.table = (tt_entry *)calloc(sizeof(tt_entry), TABLE_SIZE);
  TT.best_move = NO_MOVE;
}

int correct_retrieved_mate_score(int score, int ply_searched) {
  if(is_mate_score(score)) {
    int sign = (score >= 0) ? 1 : -1;
    return (score * sign - ply_searched) * sign; /* correct it by adding onto it how far in the search we are from the root */
  }
  return score;
}

int correct_stored_mate_score(int score, int ply_searched) {
  if(is_mate_score(score)) {
    int sign = (score >= 0) ? 1 : -1;
    return (score * sign + ply_searched) * sign; /* correct it by adding onto it how far in the search we are from the root */
  }
  return score;
}

int probe_tt_table(hash_val h, int depth, int ply_searched, int alpha, int beta) {
  tt_probes++;
  tt_entry entry = TT.table[h & (TABLE_SIZE - 1)];
  if(entry.key == h) {
    tt_hits++;
    if(entry.depth >= depth) {
      int corrected_score = correct_retrieved_mate_score(entry.score, ply_searched);
      if(entry.flags == EXACT)
        return corrected_score;
      if(entry.flags == ALPHA && corrected_score <= alpha)
        return alpha;
      if(entry.flags == BETA && corrected_score >= beta)
        return beta;
    }
    TT.best_move = entry.best_move;
  }
  else {
    TT.best_move = NO_MOVE;
  }
  return FAILED_LOOKUP;
}


// always replace
void store_entry(hash_val key, int depth, int ply_searched, int flags, int score, move_t best_move) {
  int corrected_score = correct_stored_mate_score(score, ply_searched);
  tt_entry* entry = &TT.table[key & (TABLE_SIZE - 1)];
  entry->key = key;
  entry->depth = depth;
  entry->flags = flags;
  entry->score = corrected_score;
  entry->best_move = best_move;
}

void free_tt_table() {
  free(TT.table);
}

// there's gotta be a better way to clear this
void clear_tt_table() {
  free_tt_table();
  init_tt_table();
}


/**
 * if an entry's depth is less than the search depth, then try that move first
 * otherwise if its greater or equal just use the score from that position
 * 
 * 
 * maybe just have a global variable for the best move
 * or use a class variable or something of the sorts
 * 
 * do I need to add unplayed positions to the game history?
 * yes but only check if ply from root is > 0
 * add ply from root to search
 * 
 * can use the ply from root to see if it is 0 to set the 
 * best move and eval values
 * 
 * store the best move from the last iteration so that I can interupt the 
 * current search and use that move
 * 
 * clear the TT table in between moves
 * 
 * create global abort search variable that can be set to abort the search
 * still might need to start a thread or I can check the time
 * 
 * slowness could be due to bad move ordering for king moves in the end game
 * 
 * try searching to a fixed depths with and without transposition table to see if there are bugs
 */