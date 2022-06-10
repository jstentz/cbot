#include <stddef.h>
#include <string>

#include "pieces.h"


size_t index_from_piece(piece pc) {
    return pc - 2; //this is to offset the existence of the two EMPTY (0000 and 0001)
}

bool is_sliding_piece(piece pc) {
    piece masked_pc = pc & 0xE; // add #define COLOR and #define PIECE
    if(masked_pc == ROOK || masked_pc == BISHOP || masked_pc == QUEEN) return true;
    return false;
}

piece piece_from_char(char pc) {
    return BLACK;
}

piece piece_from_move_char(char pc) {
    if(pc == 'N') return KNIGHT;
    else if(pc == 'B') return BISHOP;
    else if(pc == 'R') return ROOK;
    else if(pc == 'Q') return QUEEN;
    else return KING;
}