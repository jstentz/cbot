#include "tt.h"
#include "hashing.h"
#include "moves.h"

#include <unordered_set>
#include <cstdlib>

history_t game_history;

bool probe_game_history(hash_val h) {
    if(game_history.find(h) == game_history.end())
        return false;
    return true;
}

tt_t TT;

size_t tt_hits = 0;

size_t tt_probes = 0;

size_t checkmates = 0;

void init_tt_table() {
    TT.table = (tt_entry *)calloc(sizeof(tt_entry), TABLE_SIZE);
    TT.best_move = NO_MOVE;
}

int probe_tt_table(hash_val h, int depth, int alpha, int beta) {
    tt_probes++;
    tt_entry entry = TT.table[h % TABLE_SIZE];
    if(entry.key == h) {
        tt_hits++;
        if(entry.depth >= depth) {
            if(entry.flags == EXACT)
                return entry.score;
            if(entry.flags == ALPHA && entry.score <= alpha)
                return alpha;
            if(entry.flags == BETA && entry.score >= beta)
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
void store_entry(hash_val key, int depth, int flags, int score, move_t best_move) {
    tt_entry* entry = &TT.table[key % TABLE_SIZE];

    /* no need to replace it if we are making it worse */
    if((entry->key == key) && (entry->depth > depth)) 
        return;
    // entry->valid = true;
    entry->key = key;
    entry->depth = depth;
    entry->flags = flags;
    entry->score = score;
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