#include <stddef.h>
#include <string>

#include "include/pieces.h"

bool is_sliding_piece(piece pc) {
    piece masked_pc = pc & 0xE; // add #define COLOR and #define PIECE
    if(masked_pc == ROOK || masked_pc == BISHOP || masked_pc == QUEEN) return true;
    return false;
}

piece piece_from_char(char pc) { // unfinished
    return BLACK;
}

piece piece_from_move_char(char pc) {
    if(pc == 'N') return KNIGHT;
    else if(pc == 'B') return BISHOP;
    else if(pc == 'R') return ROOK;
    else if(pc == 'Q') return QUEEN;
    else return KING;
}