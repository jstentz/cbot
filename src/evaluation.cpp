#include "evaluation.h"
#include "board.h"
#include "bitboard.h"
#include "pieces.h"


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

    eval = 5 * cmd(losing_king) + 2 * (14 - md(losing_king, winning_king));
    return eval * perspective;
}

int evaluate(board_t *board) {
    int eval = 0;
    int piece_counts[10];
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

    square white_king_loc = board->white_king_loc;
    square black_king_loc = board->black_king_loc;

    // check if its the end game
    if(total_material_count < 2000) {
        positional_score += piece_scores[WHITE_KINGS_INDEX + 2][white_king_loc]; // access the endgame positional scores
        positional_score += piece_scores[BLACK_KINGS_INDEX + 2][black_king_loc];
    }
    else{
        positional_score += piece_scores[WHITE_KINGS_INDEX][white_king_loc]; // access the middle game positional scores
        positional_score += piece_scores[BLACK_KINGS_INDEX][black_king_loc];
    }

    // int white_king_rank = RANK(white_king_loc);
    // int white_king_file = FILE(white_king_loc);
    // int black_king_rank = RANK(black_king_loc);
    // int black_king_file = FILE(black_king_loc);
    // int rank_distance = abs(white_king_rank - black_king_rank);
    // int file_distance = abs(white_king_file - black_king_file);
    // int distance_between_kings = (rank_distance > file_distance) ? rank_distance : file_distance;

    // cout << distance_between_kings << endl;

    // double endgame_weight = 1.0 - (((double)total_material_count) / 8000.0); // total material on both sides at start = 8000

    // cout << endgame_weight << endl;
    // need to add functionality for repeat draws so it doesn't repeat in the endgame
    // int king_distance_eval = (int)(endgame_weight * (double)((7 - distance_between_kings) * 5));
    // cout << king_distance_eval << endl;
    /*
        All scores are calculated as positive meaning "good for white."
        Therefore, if it is black's turn, we have to negate our evaluation
        so that positive means "good for black."
    */
   if(piece_counts[WHITE_PAWNS_INDEX] == 0 && 
      piece_counts[BLACK_PAWNS_INDEX] == 0) {
          if(material_score >= 0) eval += mop_up_eval(board, W);
          else eval += mop_up_eval(board, B);
    }
    eval += (material_score + positional_score);
    int perspective = (board->t == W) ? 1 : -1;
    return eval * perspective;
}