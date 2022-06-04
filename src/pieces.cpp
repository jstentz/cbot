#include <stddef.h>

#include "pieces.h"


size_t index_from_piece(piece pc) {
    return pc - 2; //this is to offset the existence of the two EMPTY (0000 and 0001)
}

bool is_sliding_piece(piece pc) {
    piece masked_pc = pc & 0xE; // add #define COLOR and #define PIECE
    if(masked_pc == ROOK || masked_pc == BISHOP || masked_pc == QUEEN) return true;
    return false;
}