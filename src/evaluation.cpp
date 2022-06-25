#include "evaluation.h"
#include "board.h"
#include "bitboard.h"
#include "pieces.h"


float game_phase;

int piece_counts[10];

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
    int wp = piece_counts[WHITE_PAWNS_INDEX];
    int wn = piece_counts[WHITE_KNIGHTS_INDEX];
    int wb = piece_counts[WHITE_BISHOPS_INDEX];
    int wr = piece_counts[WHITE_ROOKS_INDEX];
    int wq = piece_counts[WHITE_QUEENS_INDEX];

    int bp = piece_counts[BLACK_PAWNS_INDEX];
    int bn = piece_counts[BLACK_KNIGHTS_INDEX];
    int bb = piece_counts[BLACK_BISHOPS_INDEX];
    int br = piece_counts[BLACK_ROOKS_INDEX];
    int bq = piece_counts[BLACK_QUEENS_INDEX];

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
    // cout << "Game phase: " << game_phase << endl;
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
    int total_material_count = 0;

    // positive number initially means it is good for white
    // when returning we will negate based on who's turn it is
    int material_score = 0; 
    int positional_score = 0;

    bitboard piece_board;
    square piece_sq;
    int piece_value;
    for(int i = 0; i < 10; i++) { // only 10 because handle kings seperately
        piece_board = board->piece_boards[i];
        piece_counts[i] = 0;
        while(piece_board) {
            piece_sq = (square)first_set_bit(piece_board);
            piece_value = piece_values[i];
            positional_score += piece_scores[i][piece_sq]; // get the correct board and index it at its square location
            material_score += piece_value;
            piece_counts[i]++; // we've seen 1 of this piece type
            total_material_count += abs(piece_value);
            REMOVE_FIRST(piece_board);
        }
    }

    if(!sufficient_checkmating_material(board)) {
        return 0;
    }
    /* make sure that piece_counts is populated (as is above) before calling */
    calculate_game_phase();

    square white_king_loc = board->white_king_loc;
    square black_king_loc = board->black_king_loc;

    int middlegame_positional = positional_score + 
                                piece_scores[WHITE_KINGS_INDEX][white_king_loc];
                                piece_scores[BLACK_KINGS_INDEX][black_king_loc];

    int endgame_positional = positional_score + 
                             piece_scores[WHITE_KINGS_INDEX + 2][white_king_loc];
                             piece_scores[BLACK_KINGS_INDEX + 2][black_king_loc];

    middlegame_eval = middlegame_positional + material_score;
    endgame_eval = endgame_positional + material_score;


    /*
        All scores are calculated as positive meaning "good for white."
        Therefore, if it is black's turn, we have to negate our evaluation
        so that positive means "good for black."
    */
    if(material_score >= 0) endgame_eval += mop_up_eval(board, W);
    else endgame_eval += mop_up_eval(board, B);

    eval = ((middlegame_eval * (256 - game_phase)) + (endgame_eval * game_phase)) / 256;

    int perspective = (board->t == W) ? 1 : -1;
    return eval * perspective;
}