#include "hashing.h"
#include "pieces.h"
#include "board.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

hash_val rand64() {
    hash_val r = 0;
    for(int i = 0; i < 64; i++) {
        if(rand() % 2 == 0) {
            r = r | (hash_val)0x1;
        }
        r = r << 1;
    }
    return r;
}

zobrist_table_t init_zobrist() {
    // srand(time(0));
    /** 
     * IMPORTANT I DON'T RESEED THIS EVERYTIME 
     * IM NOT SURE IF THIS COULD MEAN THAT THERE WOULD BE ISSUES WHEN HASHING BOARDS
     * THE SAME BOARD WILL ALWAYS HASH TO THE SAME NUMBER ACROSS PROGRAM LAUCHES
     * ... I don't think this matters though because that just means that if there
     * is a conflict (which there shouldn't be) it'll always be the same boards
     * conflicting
     */
    zobrist_table_t zobrist_table;
    for(int i = 0; i < 64; i++) {
        for(int j = 0; j < 12; j++) {
            zobrist_table.table[i][j] = rand64();
        }
    }
    zobrist_table.black_to_move = rand64();
    zobrist_table.white_king_side = rand64();
    zobrist_table.white_queen_side = rand64();
    zobrist_table.black_king_side = rand64();
    zobrist_table.black_queen_side = rand64();

    for(int i = 0; i < 8; i++) {
        zobrist_table.en_passant_file[i] = rand64();
    }
    return zobrist_table;
}

zobrist_table_t zobrist_table;

hash_val zobrist_hash(board_t *board) {
    hash_val h = 0;
    if(board->t == B) h ^= zobrist_table.black_to_move;
    for(int i = 0; i < 64; i++) {
        piece pc = board->sq_board[i];
        if(pc != EMPTY) {
            size_t j = INDEX_FROM_PIECE(pc);
            h ^= zobrist_table.table[i][j];
        }
    }
    if(board->white_king_side) h ^= zobrist_table.white_king_side;
    if(board->white_queen_side) h ^= zobrist_table.white_queen_side;
    if(board->black_king_side) h ^= zobrist_table.black_king_side;
    if(board->black_queen_side) h ^= zobrist_table.black_queen_side;

    square en_passant = board->en_passant;
    if(en_passant != NONE) {
        h ^= zobrist_table.en_passant_file[FILE(en_passant)];
    }
    return h;
}