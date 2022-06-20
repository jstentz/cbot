#include "tt.h"
#include "hashing.h"
#include "moves.h"

#include <unordered_set>

history_t game_history;

bool probe_game_history(hash_val h) {
    if(game_history.find(h) == game_history.end())
        return false;
    return true;
}