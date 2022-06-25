#include "evaluation.h"
#include "board.h"
#include "bitboard.h"
#include "pieces.h"


float game_phase;

/* this is not done */
bool sufficient_checkmating_material(board_t *board) {
    /* check for bare kings */
    if(pop_count(board->all_pieces) == 2) {
        return false;
    }
    return true;
}

/* 
    can probably incrementally update this value in the future 
    can also probably incrementally update number of pieces and positional scores
*/
void calculate_game_phase(board_t *board) {
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
    int wp = board->piece_counts[WHITE_PAWNS_INDEX];
    int wn = board->piece_counts[WHITE_KNIGHTS_INDEX];
    int wb = board->piece_counts[WHITE_BISHOPS_INDEX];
    int wr = board->piece_counts[WHITE_ROOKS_INDEX];
    int wq = board->piece_counts[WHITE_QUEENS_INDEX];

    int bp = board->piece_counts[BLACK_PAWNS_INDEX];
    int bn = board->piece_counts[BLACK_KNIGHTS_INDEX];
    int bb = board->piece_counts[BLACK_BISHOPS_INDEX];
    int br = board->piece_counts[BLACK_ROOKS_INDEX];
    int bq = board->piece_counts[BLACK_QUEENS_INDEX];

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


int mop_up_eval(board_t *board, turn winning_side) {
    // this function assumes that there are no pawns for either side
    // always returns from white's perspective
    int eval;
    square losing_king;
    square winning_king;
    int perspective;

    if(winning_side == W) {
        losing_king = board->black_king_loc;
        winning_king = board->white_king_loc;
        perspective = 1;
    }
    else {
        losing_king = board->white_king_loc;
        winning_king = board->black_king_loc;
        perspective = -1;
    }

    eval = 10 * cmd(losing_king) + 4 * (14 - md(losing_king, winning_king));
    return eval * perspective;
}

int evaluate(board_t *board) {
    /*
        eval = ((middlegame_eval * (256 - game_phase)) + (endgame_eval * game_phase)) / 256
    */
    int eval;
    int middlegame_eval;
    int endgame_eval;

    if(!sufficient_checkmating_material(board)) {
        return 0;
    }

    calculate_game_phase(board);

    square white_king_loc = board->white_king_loc;
    square black_king_loc = board->black_king_loc;

    int middlegame_positional = board->positional_score + 
                                piece_scores[WHITE_KINGS_INDEX][white_king_loc] +
                                piece_scores[BLACK_KINGS_INDEX][black_king_loc];

    int endgame_positional = board->positional_score + 
                             piece_scores[WHITE_KINGS_INDEX + 2][white_king_loc] +
                             piece_scores[BLACK_KINGS_INDEX + 2][black_king_loc];

    middlegame_eval = middlegame_positional + board->material_score;
    endgame_eval = endgame_positional + board->material_score;

    /* mop up eval for winning side */
    if(board->material_score != 0){
        if(board->material_score > 0) endgame_eval += mop_up_eval(board, W);
        else endgame_eval += mop_up_eval(board, B);
    }
    

    eval = ((middlegame_eval * (256 - game_phase)) + (endgame_eval * game_phase)) / 256;

    int perspective = (board->t == W) ? 1 : -1;
    return eval * perspective;
}