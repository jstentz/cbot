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
    srand(time(0));
    zobrist_table_t zobrist_table;
    for(int i = 0; i < 64; i++) {
        for(int j = 0; j < 12; j++) {
            zobrist_table.table[i][j] = rand64();
        }
    }
    zobrist_table.black_to_move = rand64();
    return zobrist_table;
}

static zobrist_table_t zobrist_table = init_zobrist();

hash_val zobrist_hash(board_t *board) {
    hash_val h = 0;
    if(board->t == B) h = h ^ zobrist_table.black_to_move;
    for(int i = 0; i < 64; i++) {
        piece pc = board->sq_board[i];
        if(pc != EMPTY) {
            size_t j = index_from_piece(pc);
            h = h ^ zobrist_table.table[i][j];
        }
    }
    return h;
}