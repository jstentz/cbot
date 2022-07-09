#include "evaluation.h"
#include "board.h"
#include "bitboard.h"
#include "pieces.h"

#include <cstdlib>

eval_entry *eval_table;

int eval_hits = 0;
int eval_probes = 0;

void init_eval_table() {
    eval_table = (eval_entry *)calloc(sizeof(eval_entry), EVAL_SIZE);
}

void free_eval_table() {
    free(eval_table);
}

void clear_eval_table() {
    free_eval_table();
    init_eval_table();
}

int probe_eval_table(hash_val key) {
    eval_probes++;
    eval_entry entry = eval_table[key % EVAL_SIZE];
    if(entry.key == key) {
        eval_hits++;
        return entry.score;
    }
    return FAILED_LOOKUP;
}

void store_eval_entry(hash_val key, int score) {
    eval_entry* entry = &eval_table[key % EVAL_SIZE];
    entry->key = key;
    entry->score = score;
}

float game_phase;

/* this is not done */
bool sufficient_checkmating_material() {
    /* check for bare kings */
    if(pop_count(b.all_pieces) == 2) {
        return false;
    }
    return true;
}

/* 
    can probably incrementally update this value in the future and store it in the board
    can also probably incrementally update number of pieces and positional scores
*/
void calculate_game_phase() {
    int pawn_phase = 0;
    int knight_phase = 1;
    int bishop_phase = 1;
    int rook_phase = 2;
    int queen_phase = 4;
    float total_phase = pawn_phase * 16 + 
                        knight_phase * 4 + 
                        bishop_phase * 4 + 
                        rook_phase * 4 + 
                        queen_phase * 2;

    float phase = total_phase;
    /* count the number of each piece */
    int wp = b.piece_counts[WHITE_PAWNS_INDEX];
    int wn = b.piece_counts[WHITE_KNIGHTS_INDEX];
    int wb = b.piece_counts[WHITE_BISHOPS_INDEX];
    int wr = b.piece_counts[WHITE_ROOKS_INDEX];
    int wq = b.piece_counts[WHITE_QUEENS_INDEX];

    int bp = b.piece_counts[BLACK_PAWNS_INDEX];
    int bn = b.piece_counts[BLACK_KNIGHTS_INDEX];
    int bb = b.piece_counts[BLACK_BISHOPS_INDEX];
    int br = b.piece_counts[BLACK_ROOKS_INDEX];
    int bq = b.piece_counts[BLACK_QUEENS_INDEX];

    phase -= wp * pawn_phase;
    phase -= wn * knight_phase;
    phase -= wb * bishop_phase;
    phase -= wr * rook_phase;
    phase -= wq * queen_phase;

    phase -= bp * pawn_phase;
    phase -= bn * knight_phase;
    phase -= bb * bishop_phase;
    phase -= br * rook_phase;
    phase -= bq * queen_phase;

    game_phase = (phase * 256 + (total_phase / 2)) / total_phase;
    return;
}

const int center_manhattan_distance_arr[64] = {
  6, 5, 4, 3, 3, 4, 5, 6,
  5, 4, 3, 2, 2, 3, 4, 5,
  4, 3, 2, 1, 1, 2, 3, 4,
  3, 2, 1, 0, 0, 1, 2, 3,
  3, 2, 1, 0, 0, 1, 2, 3,
  4, 3, 2, 1, 1, 2, 3, 4,
  5, 4, 3, 2, 2, 3, 4, 5,
  6, 5, 4, 3, 3, 4, 5, 6
};

int cmd(square sq) {
    return center_manhattan_distance_arr[sq];
}

int md(square sq1, square sq2) {
    int f1 = FILE(sq1);
    int f2 = FILE(sq2);
    int r1 = RANK(sq1);
    int r2 = RANK(sq2);
    return abs(r2 - r1) + abs(f2 - f1);
}


int mop_up_eval(turn winning_side) {
    // this function assumes that there are no pawns for either side
    // always returns from white's perspective
    int eval;
    square losing_king;
    square winning_king;
    int perspective;

    if(winning_side == W) {
        losing_king = b.black_king_loc;
        winning_king = b.white_king_loc;
        perspective = 1;
    }
    else {
        losing_king = b.white_king_loc;
        winning_king = b.black_king_loc;
        perspective = -1;
    }

    eval = 10 * cmd(losing_king) + 4 * (14 - md(losing_king, winning_king));
    return eval * perspective;
}

/* everything in here is scored in white's perspective until the end */
int evaluate() {
    /* probe the eval table */
    int table_score = probe_eval_table(b.board_hash);
    // cout << "probed!" << endl;
    if(table_score != FAILED_LOOKUP)
        return table_score;


    int eval;
    int middlegame_eval;
    int endgame_eval;

    if(!sufficient_checkmating_material()) {
        return 0;
    }

    calculate_game_phase();

    square white_king_loc = b.white_king_loc;
    square black_king_loc = b.black_king_loc;

    int middlegame_positional = b.positional_score + 
                                piece_scores[WHITE_KINGS_INDEX][white_king_loc] +
                                piece_scores[BLACK_KINGS_INDEX][black_king_loc];

    int endgame_positional = b.positional_score + 
                             piece_scores[WHITE_KINGS_INDEX + 2][white_king_loc] +
                             piece_scores[BLACK_KINGS_INDEX + 2][black_king_loc];

    middlegame_eval = middlegame_positional + b.material_score;
    endgame_eval = endgame_positional + b.material_score;

    /* mop up eval for winning side */
    if(b.material_score != 0){
        if(b.material_score > 0) endgame_eval += mop_up_eval(W);
        else endgame_eval += mop_up_eval(B);
    }
    
    /* add a tempo bonus to middle game */
    if(b.t == W) middlegame_eval += 10;
    else         middlegame_eval -= 10;

    eval = (((middlegame_eval * (256 - game_phase)) + (endgame_eval * game_phase)) / 256);

    int perspective = (b.t == W) ? 1 : -1;
    eval *= perspective;

    /* save the evaluation we just made */
    /* here I just need to hash the board's pieces 
    add this after I finish unmake move */
    store_eval_entry(b.board_hash, eval);
    return eval;
}