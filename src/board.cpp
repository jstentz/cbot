#include <string>
#include <stdlib.h>
#include <iostream>

#include "board.h"
#include "bitboard.h"
#include "pieces.h"
#include "attacks.h"
#include "evaluation.h"
#include "hashing.h"

void place_piece(bitboard *bb, square sq) {
    *bb = *bb | luts.pieces[sq];
    return;
}

void rem_piece(bitboard *bb, square sq) {
    *bb = *bb & ~(luts.pieces[sq]);
    return;
}

void update_boards(board_t *board) {
    board->white_pieces = (board->piece_boards[WHITE_PAWNS_INDEX]   | board->piece_boards[WHITE_KNIGHTS_INDEX] | 
                           board->piece_boards[WHITE_BISHOPS_INDEX] | board->piece_boards[WHITE_ROOKS_INDEX]   |
                           board->piece_boards[WHITE_QUEENS_INDEX]  | board->piece_boards[WHITE_KINGS_INDEX]);

    board->black_pieces = (board->piece_boards[BLACK_PAWNS_INDEX]   | board->piece_boards[BLACK_KNIGHTS_INDEX] | 
                           board->piece_boards[BLACK_BISHOPS_INDEX] | board->piece_boards[BLACK_ROOKS_INDEX]   |
                           board->piece_boards[BLACK_QUEENS_INDEX]  | board->piece_boards[BLACK_KINGS_INDEX]);

    board->all_pieces = board->white_pieces | board->black_pieces;
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
    board.en_passant = NONE;
    board.white_king_side = false; // default to false so I can change to true in decode_fen
    board.white_queen_side = false;
    board.black_king_side = false;
    board.black_queen_side = false;

    board.board_hash = 0;

    return board;
}

board_t decode_fen(string fen) {
    board_t board = zero_board();
    bitboard *place_board;
    piece pc;
    int col = 0;
    int row = 7;
    int loc;
    size_t i = 0;
    char c = fen[i];
    while(c != ' ') {
        pc = 0;
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
                if(pc == (WHITE | KING)) board.white_king_loc = (square)loc;
                else                     board.black_king_loc = (square)loc;
            }

            /* place the piece in its boards */
            board.sq_board[loc] = pc;
            place_board = &board.piece_boards[index_from_piece(pc)];
            place_piece(place_board, (square)loc);
            col += 1;
        }
        i++;
        c = fen[i];
    }
    i++;
    if(fen[i] == 'w') board.t = W;
    else              board.t = B;
    i++;
    while(i < fen.size()) {
        c = fen[i];
        if(c == 'K') board.white_king_side = true;
        if(c == 'Q') board.white_queen_side = true;
        if(c == 'k') board.black_king_side = true;
        if(c == 'q') board.black_queen_side = true;
        i++;
    }
    update_boards(&board);
    board.board_hash = zobrist_hash(&board); // hash the board initially
    return board;
}

// incomplete
string encode_fen(board_t *board) {
    piece *sq_board = board->sq_board;
    int consecutive_empty = 0;
    piece pc;
    string fen = "";
    string pc_str;
    piece color;
    piece pc_type;
    int sq;
    for(int i = 7; i >= 0; i--) {
        for(int j = 0; j < 8; j++) {
            sq = i * 8 + j;
            pc = sq_board[sq];
            if(pc == EMPTY) {
                consecutive_empty++;
                continue;
            }
            else if(consecutive_empty != 0) {
                fen += to_string(consecutive_empty);
                consecutive_empty = 0;
            }

            pc_type = PIECE(pc);
            if(pc_type == PAWN) pc_str = "p";
            else if(pc_type == KNIGHT) pc_str = "n";
            else if(pc_type == BISHOP) pc_str = "b";
            else if(pc_type == ROOK) pc_str = "r";
            else if(pc_type == QUEEN) pc_str = "q";
            else pc_str = "k";

            color = COLOR(pc);
            // if(color == WHITE) 
        }
        if(i != 0) fen += "/";
    }
    return fen;
}

pin_t get_pinned_pieces(board_t *board, square friendly_king_loc) {
    pin_t pin;
    pin.pinned_pieces = 0;
    bitboard curr_pin;
    square pinned_piece_loc;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    bitboard friendly_pieces;
    if(board->t == W) {
        opponent_rooks = board->piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = board->piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = board->piece_boards[BLACK_QUEENS_INDEX];
        friendly_pieces = board->white_pieces;
    }
    else {
        opponent_rooks = board->piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = board->piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = board->piece_boards[WHITE_QUEENS_INDEX];
        friendly_pieces = board->black_pieces;
    }
    bitboard king_rook_attacks = get_rook_attacks(friendly_king_loc, board->all_pieces);
    bitboard king_bishop_attacks = get_bishop_attacks(friendly_king_loc, board->all_pieces);
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
        rook_attacks = get_rook_attacks(pc_loc, board->all_pieces);
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
        bishop_attacks = get_bishop_attacks(pc_loc, board->all_pieces);
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
            queen_attacks = get_rook_attacks(pc_loc, board->all_pieces);
            curr_pin = queen_attacks & king_rook_attacks & friendly_pieces;
            if(curr_pin){
                pin.pinned_pieces |= curr_pin;
                pinned_piece_loc = (square)first_set_bit(curr_pin);
                pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
            }
        }
        else if(pc_diag == king_diag || pc_antidiag == king_antidiag){
            queen_attacks = get_bishop_attacks(pc_loc, board->all_pieces);
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

bitboard checking_pieces(board_t *board) {
    square friendly_king = (board->t == W) ? board->white_king_loc : board->black_king_loc;
    return attackers_from_square(board, friendly_king);
}

int in_check(bitboard attackers) {
    if(attackers == 0) return NO_CHECK;
    REMOVE_FIRST(attackers);
    if(attackers == 0) return SINGLE_CHECK;
    return DOUBLE_CHECK;
}