#include <string>
#include <stdlib.h>
#include <iostream>
#include <stack>

#include "board.h"
#include "bitboard.h"
#include "pieces.h"
#include "attacks.h"
#include "evaluation.h"
#include "hashing.h"
#include "tt.h"

board_t b;

void update_boards() {
    b.white_pieces = (b.piece_boards[WHITE_PAWNS_INDEX]   | b.piece_boards[WHITE_KNIGHTS_INDEX] | 
                      b.piece_boards[WHITE_BISHOPS_INDEX] | b.piece_boards[WHITE_ROOKS_INDEX]   |
                      b.piece_boards[WHITE_QUEENS_INDEX]  | b.piece_boards[WHITE_KINGS_INDEX]);

    b.black_pieces = (b.piece_boards[BLACK_PAWNS_INDEX]   | b.piece_boards[BLACK_KNIGHTS_INDEX] | 
                      b.piece_boards[BLACK_BISHOPS_INDEX] | b.piece_boards[BLACK_ROOKS_INDEX]   |
                      b.piece_boards[BLACK_QUEENS_INDEX]  | b.piece_boards[BLACK_KINGS_INDEX]);

    b.all_pieces = b.white_pieces | b.black_pieces;
    return;
}

board_t zero_board() {
    board_t board;
    
    for (size_t i = 0; i < 12; i++) {
        board.piece_boards[i] = 0;
    }

    board.white_pieces = 0;
    board.black_pieces = 0;
    board.all_pieces = 0;

    for (size_t i = 0; i < 64; i++) {
        board.sq_board[i] = EMPTY;
    }

    board.t = W;

    board.board_hash = 0;

    board.material_score = 0;
    board.positional_score = 0;
    
    for(int i = 0; i < 10; i++) {
        board.piece_counts[i] = 0;
    }

    stack<state_t> state_history;

    state_t state = 0;
    REM_WHITE_KING_SIDE(state);
    REM_WHITE_QUEEN_SIDE(state);
    REM_BLACK_KING_SIDE(state);
    REM_BLACK_QUEEN_SIDE(state);
    SET_EN_PASSANT_SQ(state, NONE);
    SET_LAST_CAPTURE(state, EMPTY);
    CL_FIFTY_MOVE(state);
    SET_LAST_MOVE(state, NO_MOVE);

    state_history.push(state);
    board.state_history = state_history;
    return board;
}

void decode_fen(string fen) {
    b = zero_board();
    bitboard *place_board;
    piece pc;
    int col = 0;
    int row = 7;
    int loc;
    size_t i = 0;
    char c = fen[i];
    while(c != ' ') {
        pc = EMPTY;
        loc = row * 8 + col;
        if (isdigit(c)) {
            col += c - '0'; // adds c onto loc
        }
        else if (c == '/') {
            row--;
            col = 0;
        }
        else {
            if (isupper(c)) {
                pc = pc | WHITE;
            }
            else {
                pc = pc | BLACK;
            }

            if (c == 'p' || c == 'P') {
                pc = pc | PAWN;
            }
            else if (c == 'n' || c == 'N') {
                pc = pc | KNIGHT;
            }
            else if (c == 'b' || c == 'B') {
                pc = pc | BISHOP;
            }
            else if (c == 'r' || c == 'R') {
                pc = pc | ROOK;
            }
            else if (c == 'q' || c == 'Q') {
                pc = pc | QUEEN;
            }
            else {
                pc = pc | KING;
                if(pc == (WHITE | KING)) b.white_king_loc = (square)loc;
                else                     b.black_king_loc = (square)loc;
            }

            /* place the piece in its boards */
            b.sq_board[loc] = pc;
            place_board = &b.piece_boards[INDEX_FROM_PIECE(pc)];
            PLACE_PIECE(*place_board, (square)loc);

            /* eval stuff */
            if(PIECE(pc) != KING && PIECE(pc) != EMPTY) {
                b.material_score += piece_values[INDEX_FROM_PIECE(pc)];
                b.positional_score += piece_scores[INDEX_FROM_PIECE(pc)][loc];
            }
            
            
            col += 1;
        }
        i++;
        c = fen[i];
    }

    state_t state = 0;
    i++;
    if(fen[i] == 'w') b.t = W;
    else              b.t = B;
    i++;
    while(i < fen.size()) {
        c = fen[i];
        if(c == 'K') SET_WHITE_KING_SIDE(state);
        if(c == 'Q') SET_WHITE_QUEEN_SIDE(state);
        if(c == 'k') SET_BLACK_KING_SIDE(state);
        if(c == 'q') SET_BLACK_QUEEN_SIDE(state);
        i++;
    }
    SET_EN_PASSANT_SQ(state, NONE);
    SET_LAST_CAPTURE(state, EMPTY);
    CL_FIFTY_MOVE(state);
    b.state_history.push(state);
    update_boards();
    b.board_hash = zobrist_hash(); // hash the board initially
    game_history.insert(b.board_hash); // might not need this

    /* more eval stuff */
    for(int i = 0; i < 10; i++) {
        b.piece_counts[i] = pop_count(b.piece_boards[i]);
    }
    return;
}

// incomplete
// string encode_fen(board_t *board) {
//     piece *sq_board = board->sq_board;
//     int consecutive_empty = 0;
//     piece pc;
//     string fen = "";
//     string pc_str;
//     piece color;
//     piece pc_type;
//     int sq;
//     for(int i = 7; i >= 0; i--) {
//         for(int j = 0; j < 8; j++) {
//             sq = i * 8 + j;
//             pc = sq_board[sq];
//             if(pc == EMPTY) {
//                 consecutive_empty++;
//                 continue;
//             }
//             else if(consecutive_empty != 0) {
//                 fen += to_string(consecutive_empty);
//                 consecutive_empty = 0;
//             }

//             pc_type = PIECE(pc);
//             if(pc_type == PAWN) pc_str = "p";
//             else if(pc_type == KNIGHT) pc_str = "n";
//             else if(pc_type == BISHOP) pc_str = "b";
//             else if(pc_type == ROOK) pc_str = "r";
//             else if(pc_type == QUEEN) pc_str = "q";
//             else pc_str = "k";

//             color = COLOR(pc);
//             // if(color == WHITE) 
//         }
//         if(i != 0) fen += "/";
//     }
//     return fen;
// }

pin_t get_pinned_pieces(square friendly_king_loc) {
    pin_t pin;
    pin.pinned_pieces = 0;
    bitboard curr_pin;
    square pinned_piece_loc;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    bitboard friendly_pieces;
    if(b.t == W) {
        opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
        friendly_pieces = b.white_pieces;
    }
    else {
        opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
        friendly_pieces = b.black_pieces;
    }
    bitboard king_rook_attacks = get_rook_attacks(friendly_king_loc, b.all_pieces);
    bitboard king_bishop_attacks = get_bishop_attacks(friendly_king_loc, b.all_pieces);
    bitboard rook_attacks;
    bitboard bishop_attacks;
    bitboard queen_attacks;
    square pc_loc;
    int pc_rank;
    int pc_file;
    int pc_diag;
    int pc_antidiag;
    int king_rank = RANK(friendly_king_loc);
    int king_file = FILE(friendly_king_loc);
    int king_diag = king_rank - king_file;
    int king_antidiag = king_rank + king_file;
    while(opponent_rooks) {
        pc_loc = (square)first_set_bit(opponent_rooks);
        pc_rank = RANK(pc_loc);
        pc_file = FILE(pc_loc);
        if(!(pc_rank == king_rank || pc_file == king_file)) {
            REMOVE_FIRST(opponent_rooks);
            continue;
        }
        rook_attacks = get_rook_attacks(pc_loc, b.all_pieces);
        curr_pin = rook_attacks & king_rook_attacks & friendly_pieces;
        if(curr_pin){
            pin.pinned_pieces |= curr_pin;
            pinned_piece_loc = (square)first_set_bit(curr_pin);
            pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
        }
        REMOVE_FIRST(opponent_rooks);
    }
    while(opponent_bishops) {
        pc_loc = (square)first_set_bit(opponent_bishops);
        pc_rank = RANK(pc_loc);
        pc_file = FILE(pc_loc);
        pc_diag = pc_rank - pc_file;
        pc_antidiag = pc_rank + pc_file;
        if(!(pc_diag == king_diag || pc_antidiag == king_antidiag)) {
            REMOVE_FIRST(opponent_bishops);
            continue;
        }
        bishop_attacks = get_bishop_attacks(pc_loc, b.all_pieces);
        curr_pin = bishop_attacks & king_bishop_attacks & friendly_pieces;
        if(curr_pin){
            pin.pinned_pieces |= curr_pin;
            pinned_piece_loc = (square)first_set_bit(curr_pin);
            pin.ray_at_sq[pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
        }
        REMOVE_FIRST(opponent_bishops);
    }
    while(opponent_queens) {
        pc_loc = (square)first_set_bit(opponent_queens);
        pc_rank = RANK(pc_loc);
        pc_file = FILE(pc_loc);
        pc_diag = pc_rank - pc_file;
        pc_antidiag = pc_rank + pc_file;
        if(pc_rank == king_rank || pc_file == king_file) {
            queen_attacks = get_rook_attacks(pc_loc, b.all_pieces);
            curr_pin = queen_attacks & king_rook_attacks & friendly_pieces;
            if(curr_pin){
                pin.pinned_pieces |= curr_pin;
                pinned_piece_loc = (square)first_set_bit(curr_pin);
                pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
            }
        }
        else if(pc_diag == king_diag || pc_antidiag == king_antidiag){
            queen_attacks = get_bishop_attacks(pc_loc, b.all_pieces);
            curr_pin = queen_attacks & king_bishop_attacks & friendly_pieces;
            if(curr_pin){
                pin.pinned_pieces |= curr_pin;
                pinned_piece_loc = (square)first_set_bit(curr_pin);
                pin.ray_at_sq[pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
            }
        }
        REMOVE_FIRST(opponent_queens);
    }
    return pin;
}

bitboard checking_pieces() {
    square friendly_king = (b.t == W) ? b.white_king_loc : b.black_king_loc;
    return attackers_from_square(friendly_king);
}

int in_check(bitboard attackers) {
    if(attackers == 0) return NO_CHECK;
    REMOVE_FIRST(attackers);
    if(attackers == 0) return SINGLE_CHECK;
    return DOUBLE_CHECK;
}