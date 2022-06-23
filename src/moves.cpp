#include "moves.h"
#include "bitboard.h"
#include "board.h"
#include "attacks.h"
#include "pieces.h"
#include "evaluation.h"
#include "hashing.h"
#include "debugging.h"

#include <vector>
#include <stack>
#include <iostream>
#include <string>
#include <algorithm>

// considering just giving these functions the bitboards they need
// do this after fixing move representation
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
    bitboard blocking_pieces = board->all_pieces & ~BIT_FROM_SQ(king); // the king cannot block the attack on a square behind it

    while(king_pseudomoves) {
        square loc = (square)first_set_bit(king_pseudomoves);
        if(!is_attacked(board, loc, blocking_pieces)) king_legal_moves |= BIT_FROM_SQ(loc);
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
        en_passant_bit =  BIT_FROM_SQ(en_passant_sq); // used to and with attack pattern
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
            board_without_pawns = board->all_pieces & ~(BIT_FROM_SQ(pawn)) & ~(en_passant_bit >> 8);
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
            board_without_pawns = board->all_pieces & ~(BIT_FROM_SQ(pawn)) & ~(en_passant_bit << 8);
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

move_t construct_move(int from, int to, int flags) {
    return (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12);
}

// int flags_from_fromto(int from, int to, board_t *board) {
//     piece mv_piece = board->sq_board[from];
//     piece tar_piece = board->sq_board[to];
//     if(PIECE(mv_piece) == PAWN && abs(RANK(from) - RANK(to)) == 2) {
//         return DOUBLE_PUSH;
//     }
//     else if(PIECE(mv_piece) == PAWN &&)
// }

void generate_knight_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    int from;
    int to;
    int flags;
    bitboard to_bit;
    bitboard knights;
    bitboard opponent_pieces;
    if (board->t == W) {
        knights = board->piece_boards[WHITE_KNIGHTS_INDEX];
        opponent_pieces = board->black_pieces;
    }
    else {
        knights = board->piece_boards[BLACK_KNIGHTS_INDEX];
        opponent_pieces = board->white_pieces;
    }
    bitboard knight_moves;
    bitboard knight_bit;
    while(knights) {
        from = first_set_bit(knights);
        knight_bit = BIT_FROM_SQ(from);
        if(knight_bit & pin->pinned_pieces) {
            REMOVE_FIRST(knights);
            continue;
        } // pinned knights cannot move at all
        knight_moves = generate_knight_move_bitboard((square)from, board) & check_mask;
        if(captures_only) { // filter out non-captures
            knight_moves &= opponent_pieces;
        }
        
        while(knight_moves) {
            to = first_set_bit(knight_moves);
            to_bit = BIT_FROM_SQ(to);
            if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
            else                         flags = QUIET_MOVE;
            (*curr_moves).push_back(construct_move(from, to, flags));
            REMOVE_FIRST(knight_moves);
        }
        REMOVE_FIRST(knights);
    }
    return;
}

void generate_king_moves(board_t *board, vector<move_t> *curr_moves, bool captures_only) {
    int from;
    int to;
    int flags;
    bitboard to_bit;
    bitboard kings;
    bitboard opponent_pieces;
    if (board->t == W) {
        kings = board->piece_boards[WHITE_KINGS_INDEX];
        opponent_pieces = board->black_pieces;
    }
    else {
        kings = board->piece_boards[BLACK_KINGS_INDEX];
        opponent_pieces = board->white_pieces;
    }
    bitboard king_moves;
    while(kings) {
        from = first_set_bit(kings);
        king_moves = generate_king_move_bitboard((square)from, board);
        if(captures_only) { // filter out non-captures
            king_moves &= opponent_pieces;
        }
        while(king_moves) {
            to = first_set_bit(king_moves);
            to_bit = BIT_FROM_SQ(to);
            if(to - from == 2) { // king side castle
                (*curr_moves).push_back(construct_move(from, to, KING_SIDE_CASTLE));
            }
            else if (to - from == -2) { // queen side castle
                (*curr_moves).push_back(construct_move(from, to, QUEEN_SIDE_CASTLE));
            } 
            else{
                if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
                else                         flags = QUIET_MOVE;
                (*curr_moves).push_back(construct_move(from, to, flags));
            }
            
            REMOVE_FIRST(king_moves);
        }
        REMOVE_FIRST(kings);
    }
    return;
}

void generate_pawn_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, bool pawn_check, pin_t *pin, bool captures_only) {
    int from;
    int to;
    int flags;
    bitboard to_bit;
    bitboard pin_mask;
    bitboard pawns;
    bitboard opponent_pieces; // used for captures only
    if (board->t == W) {
        pawns = board->piece_boards[WHITE_PAWNS_INDEX];
        opponent_pieces = board->black_pieces;
    }
    else {
        pawns = board->piece_boards[BLACK_PAWNS_INDEX];
        opponent_pieces = board->white_pieces;
    }
    bitboard pawn_moves;
    bitboard pawn_bit;
    bitboard en_passant_bit = 0;
    if(board->en_passant != NONE) {
        en_passant_bit = BIT_FROM_SQ(board->en_passant);
        if(pawn_check) {
            check_mask |= en_passant_bit;
        }
    }
    while(pawns) {
        from = first_set_bit(pawns);
        pawn_bit = BIT_FROM_SQ(from);
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(pawn_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
        pawn_moves = generate_pawn_move_bitboard((square)from, board) & check_mask & pin_mask;
        if(captures_only) { // only keep captures if captures_only is set
            pawn_moves &= (opponent_pieces | en_passant_bit);
        }
        while(pawn_moves) {
            to = first_set_bit(pawn_moves);
            to_bit = BIT_FROM_SQ(to);
            if(to_bit & en_passant_bit)         flags = EN_PASSANT_CAPTURE;
            else if(to_bit & opponent_pieces)   flags = NORMAL_CAPTURE;
            else                                flags = QUIET_MOVE;
            int to_rank = RANK(to);
            if(to_rank == RANK_8 || to_rank == RANK_1) {
                (*curr_moves).push_back(construct_move(from, to, flags | KNIGHT_PROMO)); // or this on to flag because we check for captures prior to this
                (*curr_moves).push_back(construct_move(from, to, flags | BISHOP_PROMO));
                (*curr_moves).push_back(construct_move(from, to, flags | ROOK_PROMO));
                (*curr_moves).push_back(construct_move(from, to, flags | QUEEN_PROMO));
            }
            else if (abs(from - to) == 16) { // double pawn push
                (*curr_moves).push_back(construct_move(from, to, DOUBLE_PUSH));
            }
            else {
                (*curr_moves).push_back(construct_move(from, to, flags)); // should already be set to quiet or capture
            }
            REMOVE_FIRST(pawn_moves);
        }
        REMOVE_FIRST(pawns);
    }
    return;
}

void generate_rook_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    int from;
    int to;
    int flags;
    bitboard to_bit;
    bitboard pin_mask;
    bitboard rooks;
    bitboard opponent_pieces;
    if (board->t == W) {
        rooks = board->piece_boards[WHITE_ROOKS_INDEX];
        opponent_pieces = board->black_pieces;
    }
    else {
        rooks = board->piece_boards[BLACK_ROOKS_INDEX];
        opponent_pieces = board->white_pieces;
    }

    bitboard rook_moves;
    bitboard rook_bit;
    while(rooks) {
        from = first_set_bit(rooks);
        rook_bit = BIT_FROM_SQ(from);
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(rook_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
        rook_moves = generate_rook_move_bitboard((square)from, board) & check_mask & pin_mask;
        if(captures_only) { // filter out for captures only
            rook_moves &= opponent_pieces;
        }
        while(rook_moves) {
            to = first_set_bit(rook_moves);
            to_bit = BIT_FROM_SQ(to);
            if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
            else                         flags = QUIET_MOVE;
            (*curr_moves).push_back(construct_move(from, to, flags));
            REMOVE_FIRST(rook_moves);
        }
        REMOVE_FIRST(rooks);
    }
    return;
}

void generate_bishop_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    int from;
    int to;
    int flags;
    bitboard to_bit;
    bitboard pin_mask;
    bitboard bishops;
    bitboard opponent_pieces;
    if (board->t == W) {
        bishops = board->piece_boards[WHITE_BISHOPS_INDEX];
        opponent_pieces = board->black_pieces;
    }
    else {
        bishops = board->piece_boards[BLACK_BISHOPS_INDEX];
        opponent_pieces = board->white_pieces;
    }

    bitboard bishop_moves;
    bitboard bishop_bit;
    while(bishops) {
        from = first_set_bit(bishops);
        bishop_bit = BIT_FROM_SQ(from);
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(bishop_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
        bishop_moves = generate_bishop_move_bitboard((square)from, board) & check_mask & pin_mask;
        if(captures_only){ // filter out the non captures
            bishop_moves &= opponent_pieces;
        }
        while(bishop_moves) {
            to = first_set_bit(bishop_moves);
            to_bit = BIT_FROM_SQ(to);
            if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
            else                         flags = QUIET_MOVE;
            (*curr_moves).push_back(construct_move(from, to, flags));
            REMOVE_FIRST(bishop_moves);
        }
        REMOVE_FIRST(bishops);
    }
    return;
}

void generate_queen_moves(board_t *board, vector<move_t> *curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
    int from;
    int to;
    int flags;
    bitboard to_bit;
    bitboard pin_mask;
    bitboard queens;
    bitboard opponent_pieces;
    if (board->t == W) {
        queens = board->piece_boards[WHITE_QUEENS_INDEX];
        opponent_pieces = board->black_pieces;
    }
    else {
        queens = board->piece_boards[BLACK_QUEENS_INDEX];
        opponent_pieces = board->white_pieces;
    }

    bitboard queen_moves;
    bitboard queen_bit;
    while(queens) {
        from = first_set_bit(queens);
        queen_bit = BIT_FROM_SQ(from);
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(queen_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
        queen_moves = generate_queen_move_bitboard((square)from, board) & check_mask & pin_mask;
        if(captures_only) { // filter out non captures
            queen_moves &= opponent_pieces;
        }
        while(queen_moves) {
            to = first_set_bit(queen_moves);
            to_bit = BIT_FROM_SQ(to);
            if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
            else                         flags = QUIET_MOVE;
            (*curr_moves).push_back(construct_move(from, to, flags));
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
void order_moves(vector<move_t> *moves, board_t *board, move_t tt_best_move) {
    // how do I assign negative scores here
    // make moves signed?
    // DOESN'T TAKE INTO ACCOUNT KING ENDGAME
    // wait the end bits might fuck this up I need to think about this
    // all move scores are assigned positive values for sorting at the end (regardless of white or black to move)
    // if a move is a capture and its moving to an unattacked square, that's probably a good move
    if((*moves).size() == 0) return;
    signed short int score;
    vector<move_t> mvs = *moves;
    piece mv_piece;
    piece tar_piece;
    int to;
    int from;
    move_t mv;
    int flags;
    int perspective = (board->t == W) ? 1 : -1;
    // maybe add a bonus for castling moves
    // add recapturing the piece that was last captured as a good bonus to check first
    // bigger bonus for the higher value piece being captured
    // just have the board store the move that was made to get to that position
    // still need to add the least_valued_attacker logic, not exactly sure how to implement
    move_t last_move = board->last_move;
    int recapture_square = -1;
    if(last_move != NO_MOVE && IS_CAPTURE(last_move)) {
        recapture_square = TO(last_move);
    }
    for(int i = 0; i < mvs.size(); i++) {
        score = 0;
        mv = mvs[i];
        if(tt_best_move != NO_MOVE && mv == tt_best_move) {
            score += 10000; // idk try the PV node first
        }
        to = TO(mv);
        from = FROM(mv);
        mv_piece = board->sq_board[from];
        if(IS_PROMO(mv)) {
            flags = FLAGS(mv);
            if(flags == KNIGHT_PROMO || flags == KNIGHT_PROMO_CAPTURE) {
                score += piece_values[WHITE_KNIGHTS_INDEX]; // just use the white knights because positive value
            }
            else if(flags == BISHOP_PROMO || flags == BISHOP_PROMO_CAPTURE) {
                score += piece_values[WHITE_BISHOPS_INDEX];
            }
            else if(flags == ROOK_PROMO || flags == ROOK_PROMO_CAPTURE) {
                score += piece_values[WHITE_ROOKS_INDEX];
            }
            else {
                score += piece_values[WHITE_QUEENS_INDEX];
            }
        }
        if(IS_CAPTURE(mv)) {
            tar_piece = board->sq_board[to];
            if(!is_attacked(board, (square)to, board->all_pieces)) {
                score += 5 * abs(piece_values[INDEX_FROM_PIECE(tar_piece)]);
            }
            else {
                score += abs(piece_values[INDEX_FROM_PIECE(tar_piece)]) - abs(piece_values[INDEX_FROM_PIECE(mv_piece)]);    
            }
        }
        else {
            /* score moves to squares attacked by pawns */
            if(is_attacked_by_pawn(board, (square)to)) 
                score -= abs(piece_values[INDEX_FROM_PIECE(mv_piece)]); // can play around with this
        }
        score += perspective * (piece_scores[INDEX_FROM_PIECE(mv_piece)][to] - piece_scores[INDEX_FROM_PIECE(mv_piece)][from]);

        /* check recapturing moves */
        if(to == recapture_square) 
            score += 5 * abs(piece_values[INDEX_FROM_PIECE(mv_piece)]); // arbitrary multiplication
        (*moves)[i] = ADD_SCORE_TO_MOVE(mv, (signed int)score); // convert to signed int to sign extend to 32 bits
    }
    std::sort(moves->begin(), moves->end(), greater<move_t>());
    return;
}

void make_move(stack<board_t> *board_stack, move_t move) {
    board_t curr_board = (*board_stack).top();
    board_t next_board = curr_board;
    hash_val h = curr_board.board_hash;

    int from = FROM(move);
    int to = TO(move);
    int flags = FLAGS(move);

    /* 
        always have to remove the piece from its square...
        if promotion, you cannot place the same piece on to square
     */
    piece moving_piece = curr_board.sq_board[from];
    bitboard *moving_piece_board = &next_board.piece_boards[INDEX_FROM_PIECE(moving_piece)];
    REM_PIECE(*moving_piece_board, from);
    next_board.sq_board[from] = EMPTY;

    /* XOR out the piece from hash value */
    h ^= zobrist_table.table[from][INDEX_FROM_PIECE(moving_piece)];
    
    /* update the king locations and castling rights */
    if(moving_piece == (WHITE | KING)) {
        next_board.white_king_loc = (square)to;
        next_board.white_king_side = false;
        next_board.white_queen_side = false;
    }
    else if(moving_piece == (BLACK | KING)) {
        next_board.black_king_loc = (square)to;
        next_board.black_king_side = false;
        next_board.black_queen_side = false;
    }
    else if(moving_piece == (WHITE | ROOK) && from == H1) {
        next_board.white_king_side = false;
    }
    else if(moving_piece == (WHITE | ROOK) && from == A1) {
        next_board.white_queen_side = false;
    }
    else if(moving_piece == (BLACK | ROOK) && from == H8) {
        next_board.black_king_side = false;
    }
    else if(moving_piece == (BLACK | ROOK) && from == A8) {
        next_board.black_queen_side = false;
    }

    /* default there to be no en passant square and set it if double pawn push */
    next_board.en_passant = NONE;

    bitboard *rook_board;
    piece captured_piece;
    bitboard *captured_board;
    piece promo_piece;
    bitboard *promo_board;
    int opponent_pawn_sq;
    switch (flags) {
        case QUIET_MOVE:
            PLACE_PIECE(*moving_piece_board, to); // place the moving piece
            next_board.sq_board[to] = moving_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)]; // place moving piece in hash value
            break;
        case DOUBLE_PUSH:
            PLACE_PIECE(*moving_piece_board, to); // place the moving piece
            next_board.sq_board[to] = moving_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)]; // place the pawn in hash value
            /* 
                if it's a double pawn push and we are starting on rank 2, its white pushing
                otherwise it is black pushing the pawn
            */
            next_board.en_passant = (RANK(from) == RANK_2) ? ((square)(from + 8)) : ((square)(from - 8));
            // hash value update for en passant square happens later
            break;
        case KING_SIDE_CASTLE:
            PLACE_PIECE(*moving_piece_board, to); // place the moving piece
            next_board.sq_board[to] = moving_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)]; // place the king in hash value
            if(from == E1) { // white king side
                rook_board = &next_board.piece_boards[WHITE_ROOKS_INDEX];
                REM_PIECE(*rook_board, H1);
                PLACE_PIECE(*rook_board, F1);
                next_board.sq_board[H1] = EMPTY;
                next_board.sq_board[F1] = WHITE | ROOK;
                h ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // remove white rook from H1
                h ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // place white rook on F1
            }
            else { // black king side
                rook_board = &next_board.piece_boards[BLACK_ROOKS_INDEX];
                REM_PIECE(*rook_board, H8);
                PLACE_PIECE(*rook_board, F8);
                next_board.sq_board[H8] = EMPTY;
                next_board.sq_board[F8] = BLACK | ROOK;
                h ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // remove black rook from H8
                h ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // place black rook on F8
            }
            break;
        case QUEEN_SIDE_CASTLE:
            PLACE_PIECE(*moving_piece_board, to); // place the moving piece
            next_board.sq_board[to] = moving_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)]; // place the king in hash value
            if(from == E1) { // white queen side
                rook_board = &next_board.piece_boards[WHITE_ROOKS_INDEX];
                REM_PIECE(*rook_board, A1);
                PLACE_PIECE(*rook_board, D1);
                next_board.sq_board[A1] = EMPTY;
                next_board.sq_board[D1] = WHITE | ROOK;
                h ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // remove white rook from A1
                h ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // place white rook on D1
            }
            else { // black queen side
                rook_board = &next_board.piece_boards[BLACK_ROOKS_INDEX];
                REM_PIECE(*rook_board, A8);
                PLACE_PIECE(*rook_board, D8);
                next_board.sq_board[A8] = EMPTY;
                next_board.sq_board[D8] = BLACK | ROOK;
                h ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // remove black rook from A8
                h ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // place black rook on D8
            }
            break;
        case NORMAL_CAPTURE:
            PLACE_PIECE(*moving_piece_board, to); // place the moving piece
            next_board.sq_board[to] = moving_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)]; // place the moving piece in hash value

            /* remove the captured piece from it's bitboard */
            captured_piece = curr_board.sq_board[to];
            captured_board = &next_board.piece_boards[INDEX_FROM_PIECE(captured_piece)];
            REM_PIECE(*captured_board, to);
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value

            /* remove castling rights if rook is captured in corner */
            if(PIECE(captured_piece) == ROOK) {
                if(to == H1) next_board.white_king_side = false;
                else if(to == A1) next_board.white_queen_side = false;
                else if(to == H8) next_board.black_king_side = false;
                else if(to == A8) next_board.black_queen_side = false;
            }
            break;
        case EN_PASSANT_CAPTURE:
            PLACE_PIECE(*moving_piece_board, to); // place the moving piece
            next_board.sq_board[to] = moving_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)]; // place the pawn in hash value

            /* distinguish between white and black en passant */
            opponent_pawn_sq = (RANK(to) == RANK_6) ? (to - 8) : (to + 8);

            /* remove the captured pawn */
            captured_piece = curr_board.sq_board[opponent_pawn_sq];
            captured_board = &next_board.piece_boards[INDEX_FROM_PIECE(captured_piece)];
            REM_PIECE(*captured_board, opponent_pawn_sq);
            next_board.sq_board[opponent_pawn_sq] = EMPTY;
            h ^= zobrist_table.table[opponent_pawn_sq][INDEX_FROM_PIECE(captured_piece)]; // remove the captured pawn from hash value
            break;
        case KNIGHT_PROMO_CAPTURE:
            /* remove the captured piece from it's bitboard */
            captured_piece = curr_board.sq_board[to];
            captured_board = &next_board.piece_boards[INDEX_FROM_PIECE(captured_piece)];
            REM_PIECE(*captured_board, to);
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
            /* fallthrough */
        case KNIGHT_PROMO:
            if(curr_board.t == W) {promo_piece = WHITE | KNIGHT; promo_board = &next_board.piece_boards[WHITE_KNIGHTS_INDEX];}
            else                  {promo_piece = BLACK | KNIGHT; promo_board = &next_board.piece_boards[BLACK_KNIGHTS_INDEX];}
            PLACE_PIECE(*promo_board, to);
            next_board.sq_board[to] = promo_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place knight in hash value
            break;
        case BISHOP_PROMO_CAPTURE:
            /* remove the captured piece from it's bitboard */
            captured_piece = curr_board.sq_board[to];
            captured_board = &next_board.piece_boards[INDEX_FROM_PIECE(captured_piece)];
            REM_PIECE(*captured_board, to);
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
            /* fallthrough */
        case BISHOP_PROMO:
            if(curr_board.t == W) {promo_piece = WHITE | BISHOP; promo_board = &next_board.piece_boards[WHITE_BISHOPS_INDEX];}
            else                  {promo_piece = BLACK | BISHOP; promo_board = &next_board.piece_boards[BLACK_BISHOPS_INDEX];}
            PLACE_PIECE(*promo_board, to);
            next_board.sq_board[to] = promo_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place bishop in hash value
            break;
        case ROOK_PROMO_CAPTURE:
            /* remove the captured piece from it's bitboard */
            captured_piece = curr_board.sq_board[to];
            captured_board = &next_board.piece_boards[INDEX_FROM_PIECE(captured_piece)];
            REM_PIECE(*captured_board, to);
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
            /* fallthrough */
        case ROOK_PROMO:
            if(curr_board.t == W) {promo_piece = WHITE | ROOK; promo_board = &next_board.piece_boards[WHITE_ROOKS_INDEX];}
            else                  {promo_piece = BLACK | ROOK; promo_board = &next_board.piece_boards[BLACK_ROOKS_INDEX];}
            PLACE_PIECE(*promo_board, to);
            next_board.sq_board[to] = promo_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place rook in hash value
            break;
        case QUEEN_PROMO_CAPTURE:
            /* remove the captured piece from it's bitboard */
            captured_piece = curr_board.sq_board[to];
            captured_board = &next_board.piece_boards[INDEX_FROM_PIECE(captured_piece)];
            REM_PIECE(*captured_board, to);
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
            /* fallthrough */
        case QUEEN_PROMO:
            if(curr_board.t == W) {promo_piece = WHITE | QUEEN; promo_board = &next_board.piece_boards[WHITE_QUEENS_INDEX];}
            else                  {promo_piece = BLACK | QUEEN; promo_board = &next_board.piece_boards[BLACK_QUEENS_INDEX];}
            PLACE_PIECE(*promo_board, to);
            next_board.sq_board[to] = promo_piece;
            h ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place queen in hash value
            break;
    }

    /* update hash value castling rights */
    if(curr_board.t == W) { // castling rights have changed
        if(curr_board.white_king_side && !next_board.white_king_side)
            h ^= zobrist_table.white_king_side;
        if(curr_board.white_queen_side && !next_board.white_queen_side)
            h ^= zobrist_table.white_queen_side;
    }
    else { 
        if(curr_board.black_king_side && !next_board.black_king_side)
            h ^= zobrist_table.black_king_side;
        if(curr_board.black_queen_side && !next_board.black_queen_side)
            h ^= zobrist_table.black_queen_side;
    }

    /* update en passant file in hash value */
    if(curr_board.en_passant != NONE)
        h ^= zobrist_table.en_passant_file[FILE(curr_board.en_passant)]; // remove the last board's en passant from hash value
    
    if(next_board.en_passant != NONE)
        h ^= zobrist_table.en_passant_file[FILE(next_board.en_passant)]; // place the current en passant file in hash value
    

    next_board.t = !next_board.t;

    /* reverse the black_to_move hash */
    h ^= zobrist_table.black_to_move;

    update_boards(&next_board);
    next_board.last_move = move;
    next_board.board_hash = h;
    (*board_stack).push(next_board);
    return;
}

void unmake_move(stack<board_t> *board_stack) {
    (*board_stack).pop();
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
        if(TO(single_move) == TO(move) && 
           FROM(single_move) != FROM(move) &&
           board->sq_board[FROM(single_move)] == board->sq_board[FROM(move)])
            conflicting_moves.push_back(single_move);
    }
    const string files = "abcdefgh";
    const string ranks = "12345678";
    const string pieces = "PNBRQK";
    string str_move;
    piece mv_piece = board->sq_board[FROM(move)];
    char piece_name = pieces[INDEX_FROM_PIECE(mv_piece) / 2];
    bool capture = IS_CAPTURE(move);
    bool promotion = IS_PROMO(move);
    size_t start_file_num = FILE(FROM(move));
    size_t start_rank_num = RANK(FROM(move));
    size_t tar_file_num = FILE(TO(move));
    size_t tar_rank_num = RANK(TO(move));
    char start_file = files[start_file_num];
    char start_rank = ranks[start_rank_num];
    char tar_file = files[tar_file_num];
    char tar_rank = ranks[tar_rank_num];
    bool file_conflict = false;
    bool rank_conflict = false;
    size_t conflict_file_num;
    size_t conflict_rank_num;
    square start = (square)FROM(move);
    square target = (square)TO(move);

    if(piece_name == 'P' && capture) {
        str_move.push_back(start_file);
    }
    else if (piece_name == 'K' && 
            (start == E1 && target == G1 || 
             start == E8 && target == G8)) {
        return "O-O"; // this won't quite work for adding check and checkmate
    }
    else if (piece_name == 'K' && 
            (start == E1 && target == C1 || 
             start == E8 && target == C8)) {
        return "O-O-O"; // this won't quite work for adding check and checkmate
    }
    else if(piece_name != 'P') {
        str_move.push_back(piece_name);
        for(move_t single_move : conflicting_moves) {
            conflict_file_num = FILE(FROM(single_move));
            conflict_rank_num = RANK(FROM(single_move));
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
        int flags = FLAGS(move);
        char promo_piece_c;
        if(flags == KNIGHT_PROMO || flags == KNIGHT_PROMO_CAPTURE) promo_piece_c = 'N';
        else if(flags == BISHOP_PROMO || flags == BISHOP_PROMO_CAPTURE) promo_piece_c = 'B';
        else if(flags == ROOK_PROMO || flags == ROOK_PROMO_CAPTURE) promo_piece_c = 'R';
        else promo_piece_c = 'Q';
        str_move.push_back(promo_piece_c);
    }
    return str_move;
}

// this doesn't work for promotions yet, but that shouldn't be a problem in opening
move_t move_from_notation(string notation, board_t *board) {
    // cout << notation << endl;
    string notation_copy = notation;
    if(notation.length() == 0) {
        cout << "Empty notation!\n";
        int y;
        cin >> y;
        exit(-1);
    }
    notation.erase(remove(notation.begin(), notation.end(), '+'), notation.end());
    vector<move_t> moves;
    generate_moves(board, &moves);
    // this is so ugly
    if(notation == "O-O") {
        for (move_t move : moves) {
            if(FLAGS(move) == KING_SIDE_CASTLE) return move;
        }
    }
    else if(notation == "O-O-O") {
        for (move_t move : moves) {
            if(FLAGS(move) == QUEEN_SIDE_CASTLE) return move;
        }
    }
    piece mv_piece;
    char c = notation[0];
    if(isupper(c)) {
        mv_piece = piece_from_move_char(c);
        notation = notation.substr(1, notation.length() - 1);
    }
    else mv_piece = PAWN;
    if(board->t == W) mv_piece |= WHITE;
    else mv_piece |= BLACK;

    string delimiter = "=";

    piece promotion_piece;
    size_t promotion_marker = notation.find(delimiter);
    if(promotion_marker == string::npos) { // didn't find = 
        promotion_piece = EMPTY;
    }
    else {
        promotion_piece = piece_from_move_char(notation.substr(promotion_marker + 1, notation.length() - promotion_marker)[0]);
        notation = notation.substr(0, promotion_marker);
    }

    if(board->t == W) {mv_piece |= WHITE; promotion_piece |= WHITE;}
    else {mv_piece |= BLACK; promotion_piece |= BLACK;}
    
    notation.erase(remove(notation.begin(), notation.end(), 'x'), notation.end());
    const string files = "abcdefgh";
    const string ranks = "12345678";

    int target_rank;
    int target_file;
    int start_rank = -1;
    int start_file = -1;
    square target_square;

    // cout << notation << endl;
    if(notation.length() == 2) { // no move conflict
        target_rank = ranks.find(notation[1]);
        target_file = files.find(notation[0]);
    }
    else if(notation.length() == 3) { // some conflict
        target_rank = ranks.find(notation[2]);
        target_file = files.find(notation[1]);

        if(isalpha(notation[0])) {
            start_file = files.find(notation[0]);
        }
        else {
            start_rank = ranks.find(notation[0]);
        }
    }
    else {
        target_rank = ranks.find(notation[3]);
        target_file = files.find(notation[2]);
        
        start_file = files.find(notation[0]);
        start_rank = ranks.find(notation[1]);
    }
    target_square = (square)(target_rank * 8 + target_file);
    // if(start_rank != -1) start_rank++;
    // if(start_file != -1) start_file++;
    // int x;
    // cin >> x;
    for (move_t move : moves) {
        if(TO(move) == target_square && board->sq_board[FROM(move)] == mv_piece) {
            if(start_rank == -1 && start_file == -1) return move;
            if(start_rank == -1 && start_file == FILE(FROM(move))) return move;
            if(start_rank == RANK(FROM(move)) && start_file == -1) return move;
            if(start_rank == RANK(FROM(move)) && start_file == FILE(FROM(move))) return move;
        }
    }
    cout << "Move: " << notation_copy << endl;
    print_squarewise(board->sq_board);
    int x;
    cout << "No match found!" << endl;
    cin >> x;
    exit(-1); // should match to a move
}