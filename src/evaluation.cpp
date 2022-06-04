#include "evaluation.h"
#include "board.h"

int evaluate(board_t *board) {
    // positive number initially means it is good for white
    // when returning we will negate based on who's turn it is
    int material_score = board->material_score; 
    int positional_score = board->piece_placement_score;
    int total_material_count = board->total_material;

    bitboard piece_board;
    square piece_sq;
    int piece_value;

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

    int white_king_rank = RANK(white_king_loc);
    int white_king_file = FILE(white_king_loc);
    int black_king_rank = RANK(black_king_loc);
    int black_king_file = FILE(black_king_loc);
    int rank_distance = abs(white_king_rank - black_king_rank);
    int file_distance = abs(white_king_file - black_king_file);
    int distance_between_kings = (rank_distance > file_distance) ? rank_distance : file_distance;

    // cout << distance_between_kings << endl;

    double endgame_weight = 1.0 - (((double)total_material_count) / 8000.0); // total material on both sides at start = 8000

    // cout << endgame_weight << endl;
    // need to add functionality for repeat draws so it doesn't repeat in the endgame
    int king_distance_eval = (int)(endgame_weight * (double)((7 - distance_between_kings) * 5));
    // cout << king_distance_eval << endl;
    /*
        All scores are calculated as positive meaning "good for white."
        Therefore, if it is black's turn, we have to negate our evaluation
        so that positive means "good for black."
    */

    int perspective = (board->t == W) ? 1 : -1;
    return ((material_score + positional_score) * perspective) + king_distance_eval;
}