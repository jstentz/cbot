#include "moves.h"
#include "bitboard.h"
#include "board.h"
#include "attacks.h"
#include "pieces.h"
#include "evaluation.h"

#include <vector>
#include <stack>
#include <iostream>
#include <string>

bitboard generate_knight_move_bitboard(square knight, board_t *board) {
    bitboard own_pieces;
    if(board->t == W)  own_pieces = board->white_pieces;
    else              own_pieces = board->black_pieces;
    
    bitboard knight_attacks = luts.knight_attacks[knight];

    return knight_attacks & ~own_pieces;
}

bitboard generate_king_move_bitboard(square king, board_t *board) {
    bitboard own_pieces;
    if(board->t == W)  {
        own_pieces = board->white_pieces; 
    }
    else {
        own_pieces = board->black_pieces;
    }
    
    bitboard king_attacks = luts.king_attacks[king];
    bitboard king_pseudomoves = king_attacks & ~own_pieces;

    if(!king_pseudomoves) return 0; // if the king has no pseudolegal moves, it cannot castle

    bitboard king_legal_moves = 0;
    bitboard blocking_pieces = board->all_pieces & ~luts.pieces[king]; // the king cannot block the attack on a square behind it

    while(king_pseudomoves) {
        square loc = (square)first_set_bit(king_pseudomoves);
        if(!is_attacked(board, loc, blocking_pieces)) king_legal_moves |= luts.pieces[loc];
        REMOVE_FIRST(king_pseudomoves);
    }

    // generate castling moves
    // maybe move these to defined constants
    
    const bitboard w_king_side_castle = 0x40;
    const bitboard w_queen_side_castle = 0x4;
    const bitboard b_king_side_castle = 0x4000000000000000;
    const bitboard b_queen_side_castle = 0x0400000000000000;

    square white_king_sq_1 = F1;
    square white_king_sq_2 = G1;
    square white_queen_sq_1 = D1;
    square white_queen_sq_2 = C1;
    square white_queen_sq_3 = B1; // this square is allowed to be attacked

    square black_king_sq_1 = F8;
    square black_king_sq_2 = G8;
    square black_queen_sq_1 = D8;
    square black_queen_sq_2 = C8;
    square black_queen_sq_3 = B8; // this square is allowed to be attacked

    bitboard king_castle = 0;
    if(board->t == W && board->white_king_loc == E1 && !is_attacked(board, E1, blocking_pieces)) {
        if(board->white_king_side && board->sq_board[H1] == (WHITE | ROOK)) {
            if(board->sq_board[white_king_sq_1] == EMPTY &&
               board->sq_board[white_king_sq_2] == EMPTY) {
                   if(!is_attacked(board, white_king_sq_1, blocking_pieces) &&
                      !is_attacked(board, white_king_sq_2, blocking_pieces))
                        king_castle |= w_king_side_castle;
               }
        }

        if(board->white_queen_side && board->sq_board[A1] == (WHITE | ROOK)) {
            if(board->sq_board[white_queen_sq_1] == EMPTY &&
               board->sq_board[white_queen_sq_2] == EMPTY &&
               board->sq_board[white_queen_sq_3] == EMPTY) {
                   if(!is_attacked(board, white_queen_sq_1, blocking_pieces) &&
                      !is_attacked(board, white_queen_sq_2, blocking_pieces))
                        king_castle |= w_queen_side_castle;
               }
        } 
    }
    else if (board->t == B && board->black_king_loc == E8 && !is_attacked(board, E8, blocking_pieces)) {
        if(board->black_king_side && board->sq_board[H8] == (BLACK | ROOK)) {
            if(board->sq_board[black_king_sq_1] == EMPTY &&
               board->sq_board[black_king_sq_2] == EMPTY) {
                   if(!is_attacked(board, black_king_sq_1, blocking_pieces) &&
                      !is_attacked(board, black_king_sq_2, blocking_pieces))
                        king_castle |= b_king_side_castle;
               }
        }

        if(board->black_queen_side && board->sq_board[A8] == (BLACK | ROOK)) {
            if(board->sq_board[black_queen_sq_1] == EMPTY &&
               board->sq_board[black_queen_sq_2] == EMPTY &&
               board->sq_board[black_queen_sq_3] == EMPTY) {
                   if(!is_attacked(board, black_queen_sq_1, blocking_pieces) &&
                      !is_attacked(board, black_queen_sq_2, blocking_pieces))
                        king_castle |= b_queen_side_castle;
               }
        } 
    }
    return king_legal_moves | king_castle;           
}

bitboard generate_pawn_move_bitboard(square pawn, board_t *board) {
    // will need to add en passant later
    bitboard enemy_pieces;
    bitboard all_pieces = board->all_pieces;
    bitboard captures;
    bitboard forward_moves;
    bitboard forward_one;
    bitboard forward_two;
    bitboard en_passant_capture;
    square en_passant_sq = board->en_passant;
    bitboard en_passant_bit = 0; // default it to zero
    size_t rank = RANK(pawn);
    bitboard opponent_rooks;
    bitboard opponent_queens;
    bitboard attackers;
    bitboard side_attackers;
    bitboard board_without_pawns;
    bitboard white_pawn_attacks;
    bitboard black_pawn_attacks;

    if(en_passant_sq != NONE) {
        en_passant_bit =  luts.pieces[en_passant_sq]; // used to and with attack pattern
    }

    if(board->t == W) {
        enemy_pieces = board->black_pieces;
        white_pawn_attacks = luts.white_pawn_attacks[pawn];
        captures = white_pawn_attacks & enemy_pieces;
        forward_one = luts.white_pawn_pushes[pawn] & ~all_pieces;
        forward_two = 0;
        if(rank == RANK_2 && forward_one) {
            forward_two = luts.white_pawn_pushes[pawn + 8] & ~all_pieces;
        }
        forward_moves = forward_one | forward_two;

        en_passant_capture = white_pawn_attacks & en_passant_bit;
        if(en_passant_capture){
            opponent_rooks = board->piece_boards[BLACK_ROOKS_INDEX];
            opponent_queens = board->piece_boards[BLACK_QUEENS_INDEX];
            board_without_pawns = board->all_pieces & ~(luts.pieces[pawn]) & ~(en_passant_bit >> 8);
            attackers = get_rook_attacks(board->white_king_loc, board_without_pawns) & (opponent_rooks | opponent_queens);
            side_attackers = attackers & luts.mask_rank[RANK(pawn)];
            if(side_attackers) {
                en_passant_capture = 0;
            }
        }
    }
    else {
        enemy_pieces = board->white_pieces;
        black_pawn_attacks = luts.black_pawn_attacks[pawn];
        captures = black_pawn_attacks & enemy_pieces;
        forward_one = luts.black_pawn_pushes[pawn] & ~all_pieces;
        forward_two = 0;
        if(rank == RANK_7 && forward_one) {
            forward_two = luts.black_pawn_pushes[pawn - 8] & ~all_pieces;
        }
        forward_moves = forward_one | forward_two;

        en_passant_capture = black_pawn_attacks & en_passant_bit;
        if(en_passant_capture){
            opponent_rooks = board->piece_boards[WHITE_ROOKS_INDEX];
            opponent_queens = board->piece_boards[WHITE_QUEENS_INDEX];
            board_without_pawns = board->all_pieces & ~(luts.pieces[pawn]) & ~(en_passant_bit << 8);
            attackers = get_rook_attacks(board->black_king_loc, board_without_pawns) & (opponent_rooks | opponent_queens);
            side_attackers = attackers & luts.mask_rank[RANK(pawn)];
            if(side_attackers) {
                en_passant_capture = 0;
            }
        }
    }

    return captures | forward_moves | en_passant_capture;
}

bitboard generate_rook_move_bitboard(square rook, board_t *board) {
    bitboard all_pieces = board->all_pieces;
    bitboard own_pieces;
    if(board->t == W)  own_pieces = board->white_pieces;
    else              own_pieces = board->black_pieces;

    bitboard rook_attacks = get_rook_attacks(rook, all_pieces);
    return rook_attacks & ~own_pieces;
}

bitboard generate_bishop_move_bitboard(square bishop, board_t *board) {
    bitboard all_pieces = board->all_pieces;
    bitboard own_pieces;

    if(board->t == W)  own_pieces = board->white_pieces;
    else              own_pieces = board->black_pieces;

    bitboard bishop_attacks = get_bishop_attacks(bishop, all_pieces);
    
    return  bishop_attacks & ~own_pieces;
}

bitboard generate_queen_move_bitboard(square queen, board_t *board) {
    return   generate_rook_move_bitboard(queen, board)
           | generate_bishop_move_bitboard(queen, board);
}

void generate_knight_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    move_t move;
    uint16_t pc_loc;
    uint16_t tar_loc;
    move.promotion_piece = EMPTY;
    bitboard knights;
    bitboard opponent_pieces;
    piece color;
    if (board->t == W) {
        knights = board->piece_boards[WHITE_KNIGHTS_INDEX];
        opponent_pieces = board->black_pieces;
        color = WHITE;
    }
    else {
        knights = board->piece_boards[BLACK_KNIGHTS_INDEX];
        opponent_pieces = board->white_pieces;
        color = BLACK;
    }
    bitboard knight_moves;
    bitboard knight_bit;
    while(knights) {
        pc_loc = first_set_bit(knights);
        knight_bit = luts.pieces[pc_loc];
        if(knight_bit & pin->pinned_pieces) {
            REMOVE_FIRST(knights);
            continue;
        } // pinned knights cannot move at all
        knight_moves = generate_knight_move_bitboard((square)pc_loc, board) & check_mask;
        knight_moves = (captures_only) ? (knight_moves & opponent_pieces) : knight_moves; // filter out non-captures
        while(knight_moves) {
            tar_loc = first_set_bit(knight_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | KNIGHT;
            move.tar_piece = board->sq_board[tar_loc];
            (*curr_moves).push_back(move);
            REMOVE_FIRST(knight_moves);
        }
        REMOVE_FIRST(knights);
    }
    return;
}

void generate_king_moves(board_t *board, vector<move_t> *curr_moves, bool captures_only) {
    move_t move;
    uint16_t pc_loc;
    uint16_t tar_loc;
    move.promotion_piece = EMPTY;
    bitboard kings;
    bitboard opponent_pieces;
    piece color;
    if (board->t == W) {
        kings = board->piece_boards[WHITE_KINGS_INDEX];
        opponent_pieces = board->black_pieces;
        color = WHITE;
    }
    else {
        kings = board->piece_boards[BLACK_KINGS_INDEX];
        opponent_pieces = board->white_pieces;
        color = BLACK;
    }
    bitboard king_moves;
    while(kings) {
        pc_loc = first_set_bit(kings);
        king_moves = generate_king_move_bitboard((square)pc_loc, board);
        king_moves = (captures_only) ? (king_moves & opponent_pieces) : king_moves; // filter out non-captures
        while(king_moves) {
            tar_loc = first_set_bit(king_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | KING;
            move.tar_piece = board->sq_board[tar_loc];
            (*curr_moves).push_back(move);
            REMOVE_FIRST(king_moves);
        }
        REMOVE_FIRST(kings);
    }
    return;
}

void generate_pawn_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, bool pawn_check, pin_t *pin, bool captures_only) {
    move_t move;
    uint16_t pc_loc;
    uint16_t tar_loc;
    move.promotion_piece = EMPTY;
    bitboard pin_mask;
    bitboard pawns;
    bitboard opponent_pieces; // used for captures only
    piece color;
    if (board->t == W) {
        pawns = board->piece_boards[WHITE_PAWNS_INDEX];
        opponent_pieces = board->black_pieces;
        color = WHITE;
    }
    else {
        pawns = board->piece_boards[BLACK_PAWNS_INDEX];
        opponent_pieces = board->white_pieces;
        color = BLACK;
    }
    bitboard pawn_moves;
    bitboard pawn_bit;
    bitboard en_passant_bit = 0;
    if(board->en_passant != NONE && pawn_check) {
        en_passant_bit = luts.pieces[board->en_passant];
        check_mask |= en_passant_bit;
    }
    while(pawns) {
        pc_loc = first_set_bit(pawns);
        pawn_bit = luts.pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(pawn_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[pc_loc];
        pawn_moves = generate_pawn_move_bitboard((square)pc_loc, board) & check_mask & pin_mask;
        pawn_moves = (captures_only) ? (pawn_moves & (opponent_pieces | en_passant_bit)) : pawn_moves; // choose between captures only or all moves
        // and it with opponent_pieces and the en_passant square if it exists to generate captures only
        while(pawn_moves) {
            tar_loc = first_set_bit(pawn_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | PAWN;
            move.tar_piece = board->sq_board[tar_loc];
            size_t tar_rank = RANK(tar_loc);
            if(tar_rank == RANK_8 || tar_rank == RANK_1) {
                piece color = (tar_rank == RANK_8) ? WHITE : BLACK;
                move.promotion_piece = color | KNIGHT;
                (*curr_moves).push_back(move);
                move.promotion_piece = color | BISHOP;
                (*curr_moves).push_back(move);
                move.promotion_piece = color | ROOK;
                (*curr_moves).push_back(move);
                move.promotion_piece = color | QUEEN;
                (*curr_moves).push_back(move);
            }
            else{
                move.promotion_piece = EMPTY;
                (*curr_moves).push_back(move);
            }
            REMOVE_FIRST(pawn_moves);
        }
        REMOVE_FIRST(pawns);
    }
    return;
}

void generate_rook_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t move;
    move.promotion_piece = EMPTY; // default to empty for sliding pieces
    bitboard pin_mask;
    bitboard rooks;
    bitboard opponent_pieces;
    piece color;
    if (board->t == W) {
        rooks = board->piece_boards[WHITE_ROOKS_INDEX];
        opponent_pieces = board->black_pieces;
        color = WHITE;
    }
    else {
        rooks = board->piece_boards[BLACK_ROOKS_INDEX];
        opponent_pieces = board->white_pieces;
        color = BLACK;
    }

    bitboard rook_moves;
    bitboard rook_bit;
    while(rooks) {
        pc_loc = first_set_bit(rooks);
        rook_bit = luts.pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(rook_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[pc_loc];
        rook_moves = generate_rook_move_bitboard((square)pc_loc, board) & check_mask & pin_mask;
        rook_moves = (captures_only) ? (rook_moves & opponent_pieces) : rook_moves; // filter out non-captures
        while(rook_moves) {
            tar_loc = first_set_bit(rook_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | ROOK;
            move.tar_piece = board->sq_board[tar_loc];
            (*curr_moves).push_back(move);
            REMOVE_FIRST(rook_moves);
        }
        REMOVE_FIRST(rooks);
    }
    return;
}

void generate_bishop_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t move;
    move.promotion_piece = EMPTY; // default to empty for sliding pieces
    bitboard pin_mask;
    bitboard bishops;
    bitboard opponent_pieces;
    piece color;
    if (board->t == W) {
        bishops = board->piece_boards[WHITE_BISHOPS_INDEX];
        opponent_pieces = board->black_pieces;
        color = WHITE;
    }
    else {
        bishops = board->piece_boards[BLACK_BISHOPS_INDEX];
        opponent_pieces = board->white_pieces;
        color = BLACK;
    }

    bitboard bishop_moves;
    bitboard bishop_bit;
    while(bishops) {
        pc_loc = first_set_bit(bishops);
        bishop_bit = luts.pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(bishop_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[pc_loc];
        bishop_moves = generate_bishop_move_bitboard((square)pc_loc, board) & check_mask & pin_mask;
        bishop_moves = (captures_only) ? (bishop_moves & opponent_pieces) : bishop_moves; // filter out non-captures
        while(bishop_moves) {
            tar_loc = first_set_bit(bishop_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | BISHOP;
            move.tar_piece = board->sq_board[tar_loc];
            (*curr_moves).push_back(move);
            REMOVE_FIRST(bishop_moves);
        }
        REMOVE_FIRST(bishops);
    }
    return;
}

void generate_queen_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t move;
    move.promotion_piece = EMPTY; // default to empty for sliding pieces
    bitboard pin_mask;
    bitboard queens;
    bitboard opponent_pieces;
    piece color;
    if (board->t == W) {
        queens = board->piece_boards[WHITE_QUEENS_INDEX];
        opponent_pieces = board->black_pieces;
        color = WHITE;
    }
    else {
        queens = board->piece_boards[BLACK_QUEENS_INDEX];
        opponent_pieces = board->white_pieces;
        color = BLACK;
    }

    bitboard queen_moves;
    bitboard queen_bit;
    while(queens) {
        pc_loc = first_set_bit(queens);
        queen_bit = luts.pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(queen_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[pc_loc];
        queen_moves = generate_queen_move_bitboard((square)pc_loc, board) & check_mask & pin_mask;
        queen_moves = (captures_only) ? (queen_moves & opponent_pieces) : queen_moves; // filter out non-captures
        while(queen_moves) {
            tar_loc = first_set_bit(queen_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | QUEEN;
            move.tar_piece = board->sq_board[tar_loc];
            (*curr_moves).push_back(move);
            REMOVE_FIRST(queen_moves);
        }
        REMOVE_FIRST(queens);
    }
    return;
}

void generate_moves(board_t *board, vector<move_t> *curr_moves, bool captures_only) {
    bitboard check_pieces = checking_pieces(board);
    bitboard capture_mask = 0xFFFFFFFFFFFFFFFF;
    bitboard push_mask = 0xFFFFFFFFFFFFFFFF;
    square friendly_king_loc = (board->t == W) ? board->white_king_loc : board->black_king_loc;
    int check = in_check(check_pieces);
    if(check == DOUBLE_CHECK) {
        generate_king_moves(board, curr_moves, captures_only);
        return;
    }
    else if (check == SINGLE_CHECK) {
        capture_mask = check_pieces;
        square sq = (square)first_set_bit(check_pieces);
        if(is_sliding_piece(board->sq_board[sq])) {
            push_mask = opponent_slider_rays_to_square(board, friendly_king_loc);
        }
        else {
            push_mask = 0;
        }
    }
    bool pawn_check = (check_pieces & (board->piece_boards[WHITE_PAWNS_INDEX] | board->piece_boards[BLACK_PAWNS_INDEX])) != 0;
    bitboard check_mask = push_mask | capture_mask;
    pin_t pin = get_pinned_pieces(board, friendly_king_loc); // maybe change this so that the board holds the pinned pieces info
    generate_pawn_moves(board, curr_moves, check_mask, pawn_check, &pin, captures_only);
    generate_knight_moves(board, curr_moves, check_mask, &pin, captures_only);
    generate_bishop_moves(board, curr_moves, check_mask, &pin, captures_only);
    generate_rook_moves(board, curr_moves, check_mask, &pin, captures_only);
    generate_queen_moves(board, curr_moves, check_mask, &pin, captures_only);
    generate_king_moves(board, curr_moves, captures_only);
    return;
}

// thinking about adding en passant to the move
// if you move to a square that is attacked by a lesser-valued piece, put it last
void order_moves(vector<move_t> *moves) {
    vector<move_t> new_order;
    for(move_t move : (*moves)) {
        if(move.promotion_piece != EMPTY){
            new_order.insert(new_order.begin(), move);
        }
        else if(move.tar_piece != EMPTY && (abs(piece_values[move.tar_piece]) >= abs(piece_values[move.mv_piece]))){
            new_order.insert(new_order.begin(), move);
        }
        else new_order.push_back(move);
    }
    *moves = new_order;
    return;
}

// requires that the input moves are captures only
void order_captures(vector<move_t> *moves) {
    return;
}

/*
 * Goes from a move struct to the correct notation, given a move, a list of 
 * legal moves in the position, and the state of the board.
 */
string notation_from_move(move_t move, vector<move_t> all_moves, board_t *board) {
    // conflicting doesn't work for knights right now
    // need to update for check (+) and checkmate (#)
    // need to add castling
    vector<move_t> conflicting_moves;
    for (move_t single_move : all_moves) {
        if(single_move.target == move.target     && 
           single_move.mv_piece == move.mv_piece &&
           single_move.start != move.start) 
        conflicting_moves.push_back(single_move);
    }
    const string files = "abcdefgh";
    const string ranks = "12345678";
    const string pieces = "PNBRQK";
    string str_move;
    char piece_name = pieces[index_from_piece(move.mv_piece) / 2];
    bool capture = (move.tar_piece == EMPTY) ? false : true;
    if(move.target == board->en_passant && (move.mv_piece & 0xE) == PAWN) capture = true;
    bool promotion = (move.promotion_piece == EMPTY) ? false : true;
    size_t start_file_num = move.start % 8;
    size_t start_rank_num = move.start / 8;
    size_t tar_file_num = move.target % 8;
    size_t tar_rank_num = move.target / 8;
    char start_file = files[start_file_num];
    char start_rank = ranks[start_rank_num];
    char tar_file = files[tar_file_num];
    char tar_rank = ranks[tar_rank_num];
    bool file_conflict = false;
    bool rank_conflict = false;
    size_t conflict_file_num;
    size_t conflict_rank_num;

    if(piece_name == 'P' && capture) {
        str_move.push_back(start_file);
    }
    else if (piece_name == 'K' && 
            (move.start == E1 && move.target == G1 || 
             move.start == E8 && move.target == G8)) {
        return "O-O"; // this won't quite work for adding check and checkmate
    }
    else if (piece_name == 'K' && 
            (move.start == E1 && move.target == C1 || 
             move.start == E8 && move.target == C8)) {
        return "O-O-O"; // this won't quite work for adding check and checkmate
    }
    else if(piece_name != 'P') {
        str_move.push_back(piece_name);
        for(move_t single_move : conflicting_moves) {
            conflict_file_num = single_move.start % 8;
            conflict_rank_num = single_move.start / 8;
            if(conflict_file_num == start_file_num) file_conflict = true;
            else if(conflict_rank_num == start_rank_num) rank_conflict = true;
        }
        if(file_conflict) str_move.push_back(start_rank);
        if(rank_conflict) str_move.push_back(start_file);
        if(!file_conflict && !rank_conflict && 
           conflicting_moves.size() > 0 && piece_name == 'N') {
            str_move.push_back(start_file);
        }
    }

    if(capture) str_move.push_back('x');
    str_move.push_back(tar_file);
    str_move.push_back(tar_rank);
    if(promotion) {
        str_move.push_back('=');
        str_move.push_back(
                pieces[index_from_piece(move.promotion_piece) / 2]);
    }
    return str_move;
}

board_t *make_move(board_t *board, move_t *move) {
    board_t *next_board = (board_t *)malloc(sizeof(board_t));
    memcpy(next_board, board, sizeof(board_t));

    square start = move->start;
    square target = move->target;

    piece mv_piece = move->mv_piece;
    piece tar_piece = move->tar_piece;

    bitboard *mv_board = &next_board->piece_boards[index_from_piece(mv_piece)];

    /* remove the moving piece from location score */
    next_board->piece_placement_score -= piece_scores[index_from_piece(mv_piece)][start];
    
    // always remove the piece from its board no matter what
    rem_piece(mv_board, start);

    /* update king locations */
    if (mv_piece == (WHITE | KING)) next_board->white_king_loc = target;
    else if (mv_piece == (BLACK | KING)) next_board->black_king_loc = target;

    /* check for castling move */
    bitboard *castling_rook;
    if(mv_piece == (WHITE | KING) && (start == E1) && (target == G1)) {
        castling_rook = &next_board->piece_boards[WHITE_ROOKS_INDEX];
        rem_piece(castling_rook, H1);
        place_piece(castling_rook, F1);
        next_board->sq_board[H1] = EMPTY;
        next_board->sq_board[F1] = WHITE | ROOK;
        next_board->white_king_side = false;

        next_board->piece_placement_score -= piece_scores[WHITE_ROOKS_INDEX][H1]; /* rook is no longer in the H1 */
        next_board->piece_placement_score += piece_scores[WHITE_ROOKS_INDEX][F1]; /* rook is now on F1 */
    }
    else if(mv_piece == (WHITE | KING) && (start == E1) && (target == C1)) {
        castling_rook = &next_board->piece_boards[WHITE_ROOKS_INDEX];
        rem_piece(castling_rook, A1);
        place_piece(castling_rook, D1);
        next_board->sq_board[A1] = EMPTY;
        next_board->sq_board[D1] = WHITE | ROOK;
        next_board->white_queen_side = false;

        next_board->piece_placement_score -= piece_scores[WHITE_ROOKS_INDEX][A1]; /* rook is no longer in the A1 */
        next_board->piece_placement_score += piece_scores[WHITE_ROOKS_INDEX][D1]; /* rook is now on D1 */
    }
    else if(mv_piece == (BLACK | KING) && (start == E8) && (target == G8)) {
        castling_rook = &next_board->piece_boards[BLACK_ROOKS_INDEX];
        rem_piece(castling_rook, H8);
        place_piece(castling_rook, F8);
        next_board->sq_board[H8] = EMPTY;
        next_board->sq_board[F8] = BLACK | ROOK;
        next_board->black_king_side = false;

        next_board->piece_placement_score -= piece_scores[BLACK_ROOKS_INDEX][H8]; /* rook is no longer in the H8 */
        next_board->piece_placement_score += piece_scores[BLACK_ROOKS_INDEX][F8]; /* rook is now on F8 */
    }
    else if(mv_piece == (BLACK | KING) && (start == E8) && (target == C8)) {
        castling_rook = &next_board->piece_boards[BLACK_ROOKS_INDEX];
        rem_piece(castling_rook, A8);
        place_piece(castling_rook, D8);
        next_board->sq_board[A8] = EMPTY;
        next_board->sq_board[D8] = BLACK | ROOK;
        next_board->black_queen_side = false;

        next_board->piece_placement_score -= piece_scores[BLACK_ROOKS_INDEX][A8]; /* rook is no longer in the A8 */
        next_board->piece_placement_score += piece_scores[BLACK_ROOKS_INDEX][D8]; /* rook is now on D8 */
    }

    /* check if promoting move */
    if(move->promotion_piece != EMPTY) {
        /* remove the pawn from the material scores */
        next_board->material_score -= piece_values[index_from_piece(mv_piece)];
        next_board->total_material -= abs(piece_values[index_from_piece(mv_piece)]);

        mv_piece = move->promotion_piece;
        mv_board = &next_board->piece_boards[index_from_piece(mv_piece)];

        /* add the promoting piece from the material scores */
        next_board->material_score += piece_values[index_from_piece(mv_piece)];
        next_board->total_material += abs(piece_values[index_from_piece(mv_piece)]);
    }

    place_piece(mv_board, target);

    /* check for capturing move */
    if(tar_piece != EMPTY) {
        bitboard *captured_board = &next_board->piece_boards[index_from_piece(tar_piece)];
        rem_piece(captured_board, target);

        next_board->material_score -= piece_values[index_from_piece(tar_piece)]; /* remove the captured piece from material */
        next_board->piece_placement_score -= piece_scores[index_from_piece(tar_piece)][target]; /* remove the piece from the placement score */
        next_board->total_material -= abs(piece_values[index_from_piece(tar_piece)]);
    }

    next_board->sq_board[start] = EMPTY;
    next_board->sq_board[target] = mv_piece; // mv_piece will be updated to queen if promoting move

    /* check for en passant move to remove the pawn being captured en passant */
    if(mv_piece == (WHITE | PAWN) && target == next_board->en_passant) {
        bitboard *black_pawns = &next_board->piece_boards[BLACK_PAWNS_INDEX];
        square pawn_square = (square)((int)next_board->en_passant - 8);
        rem_piece(black_pawns, pawn_square);
        next_board->sq_board[pawn_square] = EMPTY;

        next_board->material_score -= piece_values[BLACK_PAWNS_INDEX]; /* remove the captured pawn from material */
        next_board->piece_placement_score -= piece_scores[BLACK_PAWNS_INDEX][pawn_square]; /* remove the pawn from the placement score */
        next_board->total_material -= abs(piece_values[BLACK_PAWNS_INDEX]);
    }
    else if(mv_piece == (BLACK | PAWN) && target == next_board->en_passant) {
        bitboard *white_pawns = &next_board->piece_boards[WHITE_PAWNS_INDEX];
        square pawn_square = (square)((int)next_board->en_passant + 8);
        rem_piece(white_pawns, pawn_square);
        next_board->sq_board[pawn_square] = EMPTY;

        next_board->material_score -= piece_values[WHITE_PAWNS_INDEX]; /* remove the captured pawn from material */
        next_board->piece_placement_score -= piece_scores[WHITE_PAWNS_INDEX][pawn_square]; /* remove the pawn from the placement score */
        next_board->total_material -= abs(piece_values[WHITE_PAWNS_INDEX]);
    }

    /* Update en passant squares */
    if(mv_piece == (WHITE | PAWN) && target - start == 16) {
        next_board->en_passant = (square)(start + 8);
    }
    else if(mv_piece == (BLACK | PAWN) && start - target == 16) {
        next_board->en_passant = (square)(target + 8);
    }
    else {
        next_board->en_passant = NONE;
    }

    /* Update castling rights for king moves */
    if(mv_piece == (WHITE | KING)) {
        next_board->white_king_side = false;
        next_board->white_queen_side = false;
    }
    else if(mv_piece == (BLACK | KING)) {
        next_board->black_king_side = false;
        next_board->black_queen_side = false;
    }
    /* Update castling rights for rook moves */
    else if(mv_piece == (WHITE | ROOK) && start == H1) {
        next_board->white_king_side = false;
    }
    else if(mv_piece == (WHITE | ROOK) && start == A1) {
        next_board->white_queen_side = false;
    }
    else if(mv_piece == (BLACK | ROOK) && start == H8) {
        next_board->black_king_side = false;
    }
    else if(mv_piece == (BLACK | ROOK) && start == A8) {
        next_board->black_queen_side = false;
    }

    next_board->t = !next_board->t;
    update_boards(next_board);

    /* update the moving pieces location score note that mv_piece will be the promoted piece */
    next_board->piece_placement_score += piece_scores[index_from_piece(mv_piece)][target];

    return next_board;
}

void unmake_move(stack<board_t *> *board_stack) {
    free((*board_stack).top());
    (*board_stack).pop();
    return;
}