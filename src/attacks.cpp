#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "pieces.h"

#include <stddef.h>


lut_t init_LUT () {
    lut_t luts;

    bitboard rank = 0x00000000000000FF;
    bitboard file = 0x0101010101010101;
    /* creating rank and file masks and clears */
    for(size_t i = 0; i < 8; i++) {
        luts.mask_rank[i] = rank;
        luts.clear_rank[i] = ~rank;
        luts.mask_file[i] = file;
        luts.clear_file[i] = ~file;

        rank = rank << 8;
        file = file << 1;
    }

    /* creating diagonal masks */
    /* a diagonal is identified by file - rank if file >= rank else 15 - (rank - file) */
    luts.mask_diagonal[0] = 0x8040201008040201;
    luts.mask_diagonal[1] = 0x0080402010080402;
    luts.mask_diagonal[2] = 0x0000804020100804;
    luts.mask_diagonal[3] = 0x0000008040201008;
    luts.mask_diagonal[4] = 0x0000000080402010;
    luts.mask_diagonal[5] = 0x0000000000804020;
    luts.mask_diagonal[6] = 0x0000000000008040;
    luts.mask_diagonal[7] = 0x0000000000000080;
    luts.mask_diagonal[8] = 0x0100000000000000;
    luts.mask_diagonal[9] = 0x0201000000000000;
    luts.mask_diagonal[10] = 0x0402010000000000;
    luts.mask_diagonal[11] = 0x0804020100000000;
    luts.mask_diagonal[12] = 0x1008040201000000;
    luts.mask_diagonal[13] = 0x2010080402010000;
    luts.mask_diagonal[14] = 0x4020100804020100;

    /* creating antidiagonal masks */
    /* an antidiagonal is identified by (7 - file) - rank if file >= rank else rank - (7 - file) + 1*/
    luts.mask_antidiagonal[0] = 0x0102040810204080;
    luts.mask_antidiagonal[1] = 0x0001020408102040;
    luts.mask_antidiagonal[2] = 0x0000010204081020;
    luts.mask_antidiagonal[3] = 0x0000000102040810; 
    luts.mask_antidiagonal[4] = 0x0000000001020408;
    luts.mask_antidiagonal[5] = 0x0000000000010204;
    luts.mask_antidiagonal[6] = 0x0000000000000102;
    luts.mask_antidiagonal[7] = 0x0000000000000001;
    luts.mask_antidiagonal[8] = 0x8000000000000000;
    luts.mask_antidiagonal[9] = 0x4080000000000000;
    luts.mask_antidiagonal[10] = 0x2040800000000000;
    luts.mask_antidiagonal[11] = 0x1020408000000000;
    luts.mask_antidiagonal[12] = 0x0810204080000000;
    luts.mask_antidiagonal[13] = 0x0408102040800000;
    luts.mask_antidiagonal[14] = 0x0204081020408000;

    /* creating piece masks */
    bitboard piece = 0x0000000000000001;
    for(size_t i = 0; i < 64; i++) {
        luts.pieces[i] = piece;
        piece = piece << 1;
    }

    /* creating knight_attacks LUT */
    bitboard spot_1_clip = luts.clear_file[FILE_H] & luts.clear_file[FILE_G];
    bitboard spot_2_clip = luts.clear_file[FILE_H];
    bitboard spot_3_clip = luts.clear_file[FILE_A];
    bitboard spot_4_clip = luts.clear_file[FILE_A] & luts.clear_file[FILE_B];

    bitboard spot_5_clip = spot_4_clip;
    bitboard spot_6_clip = spot_3_clip;
    bitboard spot_7_clip = spot_2_clip;
    bitboard spot_8_clip = spot_1_clip;

    bitboard spot_1;
    bitboard spot_2;
    bitboard spot_3;
    bitboard spot_4;
    bitboard spot_5;
    bitboard spot_6;
    bitboard spot_7;
    bitboard spot_8;

    bitboard knight;
    for(size_t sq = 0; sq < 64; sq++) {
        knight = luts.pieces[sq];
        spot_1 = (knight & spot_1_clip) >> 6;
        spot_2 = (knight & spot_2_clip) >> 15;
        spot_3 = (knight & spot_3_clip) >> 17;
        spot_4 = (knight & spot_4_clip) >> 10;

        spot_5 = (knight & spot_5_clip) << 6;
        spot_6 = (knight & spot_6_clip) << 15;
        spot_7 = (knight & spot_7_clip) << 17;
        spot_8 = (knight & spot_8_clip) << 10;
        luts.knight_attacks[sq] = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                                   spot_7 | spot_8;
    }

    /* creating king_attacks LUT */
    spot_1_clip = luts.clear_file[FILE_A];
    spot_3_clip = luts.clear_file[FILE_H];
    spot_4_clip = luts.clear_file[FILE_H];

    spot_5_clip = luts.clear_file[FILE_H];
    spot_7_clip = luts.clear_file[FILE_A];
    spot_8_clip = luts.clear_file[FILE_A];

    bitboard king;
    for(size_t sq = 0; sq < 64; sq++) {
        king = luts.pieces[sq];
        spot_1 = (king & spot_1_clip) << 7;
        spot_2 = king << 8;
        spot_3 = (king & spot_3_clip) << 9;
        spot_4 = (king & spot_4_clip) << 1;

        spot_5 = (king & spot_5_clip) >> 7;
        spot_6 = king >> 8;
        spot_7 = (king & spot_7_clip) >> 9;
        spot_8 = (king & spot_8_clip) >> 1;
        luts.king_attacks[sq] = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                                 spot_7 | spot_8;
    }

    /* creating white_pawn_attacks LUT */
    bitboard pawn;
    spot_1_clip = luts.clear_file[FILE_A];
    spot_4_clip = luts.clear_file[FILE_H];
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts.pieces[sq];
        spot_1 = (pawn & spot_1_clip) << 7;
        spot_4 = (pawn & spot_4_clip) << 9;
        luts.white_pawn_attacks[sq] = spot_1 | spot_4;
    }

    /* creating black_pawn_attacks LUT */
    spot_1_clip = luts.clear_file[FILE_A];
    spot_4_clip = luts.clear_file[FILE_H];
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts.pieces[sq];
        spot_1 = (pawn & spot_4_clip) >> 7;
        spot_4 = (pawn & spot_1_clip) >> 9;
        luts.black_pawn_attacks[sq] = spot_1 | spot_4;
    }

    /* 
        creating white_pawn_pushes LUT 
        does not include double pawn pushes for 2nd rank pawns
        that calculation is done in the generate_pawn_moves function
    */
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts.pieces[sq];
        spot_2 = pawn << 8;
        luts.white_pawn_pushes[sq] = spot_2;
    }

    /* 
        creating black_pawn_pushes LUT 
        does not include double pawn pushes for 7th rank pawns
        that calculation is done in the generate_pawn_moves function
    */
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts.pieces[sq];
        spot_2 = pawn >> 8;
        luts.black_pawn_pushes[sq] = spot_2;
    }

    /* creating rank_attacks LUT */  
    size_t LSB_to_first_1;
    size_t LSB_to_second_1;
    bitboard mask_1 = 0x1;
    bitboard rank_attack_mask = 0xFF; // shift this at end to get attacks
    for(size_t sq = 0; sq < 64; sq++) {
        size_t file = FILE(sq); // will have to multiply later to shift the bitboard into the right spot
        for(size_t pattern = 0; pattern < 256; pattern++) {
            LSB_to_first_1 = 0;
            LSB_to_second_1 = 7;
            for(size_t i = 0; i < file; i++) {
                if((pattern >> i) & mask_1 == 1) {
                    LSB_to_first_1 = i;
                }
            }
            for(size_t i = file + 1; i < 8; i++) {
                if((pattern >> i) & mask_1 == 1) {
                    LSB_to_second_1 = i;
                    break;
                }
            }
            bitboard unplaced_mask = rank_attack_mask >> (7 - (LSB_to_second_1 - LSB_to_first_1));
            luts.rank_attacks[sq][pattern] = (unplaced_mask << (sq - (file - LSB_to_first_1))) 
                                              & ~luts.pieces[sq]; // removes piece square
        }
    }

    /* creating file_attacks LUT */
    for(size_t i = 0; i < 8; i++) {
        for(size_t j = 0; j < 8; j++){
            for(size_t pattern = 0; pattern < 256; pattern++) {
                luts.file_attacks[i*8 + j][pattern] = rotate_90_clockwise(luts.rank_attacks[j*8 + (7-i)][pattern]);
            }
        }
        
    }

    /* creating diagonal_attacks LUT */
    size_t rotated_rank;
    size_t rotated_sq;
    size_t diag;
    for(size_t i = 0; i < 8; i++) {
        for(size_t j = 0; j < 8; j++){
            if(i >= j) rotated_rank = i - j;
            else       rotated_rank = 8 - (j - i);
            rotated_sq = rotated_rank * 8 + j;
            if(j >= i) diag = j - i;
            else       diag = 15 - (i - j);
            size_t sq = i * 8 + j;
            for(size_t pattern = 0; pattern < 256; pattern++) {
                /* still need to mask the correct diagonal */
                luts.diagonal_attacks[sq][pattern] = 
                    undo_pseudo_rotate_45_clockwise(luts.rank_attacks[rotated_sq][pattern])
                    & luts.mask_diagonal[diag] & ~luts.pieces[sq]; // removes piece square
            }
        }
        
    }

    /* creating antidiagonal_attacks LUT */
    for(size_t i = 0; i < 8; i++) {
        for(size_t j = 0; j < 8; j++){
            if(j > (7 - i)) diag = 15 - (j - (7 - i));
            else             diag = (7 - i) - j;
            if(diag == 0)     rotated_rank = 0;
            else if(diag <= 7) rotated_rank = 8 - diag;
            else              rotated_rank = 8 - (diag - 7);
            rotated_sq = rotated_rank * 8 + j;
            size_t sq = i * 8 + j;
            for(size_t pattern = 0; pattern < 256; pattern++) {
                /* still need to mask the correct diagonal */
                luts.antidiagonal_attacks[sq][pattern] = 
                    undo_pseudo_rotate_45_anticlockwise(luts.rank_attacks[rotated_sq][pattern])
                    & luts.mask_antidiagonal[diag] & ~luts.pieces[sq]; // removes piece square
            }
        }
        
    }
    return luts;
}

lut_t luts;

bitboard get_knight_attacks(square knight) {
    // doesn't depend on the current position
    return luts.knight_attacks[knight];
}

bitboard get_king_attacks(square king) {
    // doesn't depend on the current position
    return luts.king_attacks[king];
}

bitboard get_pawn_attacks(square pawn, turn side) {
    // depends on who's turn it is to move
    if(side == W) return luts.white_pawn_attacks[pawn];
    return luts.black_pawn_attacks[pawn];
}

bitboard get_rook_attacks(square rook, bitboard all_pieces) {
    // depends on the placement of all_pieces on the board
    // could leave out the white king when calculating blacks attack maps
    size_t rook_rank = RANK(rook);
    size_t rook_file = FILE(rook);
    bitboard rank_pattern = (all_pieces & luts.mask_rank[rook_rank]) >> (rook_rank * 8);
    bitboard file_pattern = rotate_90_anticlockwise(all_pieces & luts.mask_file[rook_file]) >> (rook_file * 8);
    return luts.rank_attacks[rook][rank_pattern] |
           luts.file_attacks[rook][file_pattern];
}

bitboard get_bishop_attacks(square bishop, bitboard all_pieces) {
    // depends on the placement of all_pieces on the board
    // could leave out the white king when calculating blacks attack maps
    // this code is literal dog shit please clean it up later jason...
    size_t bishop_rank = RANK(bishop);
    size_t bishop_file = FILE(bishop);
    size_t bishop_diag;
    if(bishop_file >= bishop_rank) bishop_diag = bishop_file - bishop_rank;
    else                           bishop_diag = 15 - (bishop_rank - bishop_file);
    // do same for antidiag
    size_t bishop_antidiag;
    if(bishop_file > (7 - bishop_rank)) bishop_antidiag = 15 - (bishop_file - (7 - bishop_rank));
    else                                bishop_antidiag = (7 - bishop_rank) - bishop_file;

    size_t rotated_rank;
    if(bishop_diag == 0) rotated_rank = 0;
    else if (bishop_diag < 8) rotated_rank = 8 - bishop_diag;
    else rotated_rank = 8 - (bishop_diag - 7);
    bitboard diag_pattern = pseudo_rotate_45_clockwise(all_pieces & luts.mask_diagonal[bishop_diag]) >> (8 * rotated_rank);
    size_t antirotated_rank;
    if(bishop_antidiag == 0)      antirotated_rank = 0;
    else if(bishop_antidiag < 8)  antirotated_rank = 8 - bishop_antidiag;
    else                          antirotated_rank = 8 - (bishop_antidiag - 7);
    bitboard antidiag_pattern = pseudo_rotate_45_anticlockwise(all_pieces & luts.mask_antidiagonal[bishop_antidiag]) >> (8 * antirotated_rank);
    return luts.diagonal_attacks[bishop][diag_pattern] |
           luts.antidiagonal_attacks[bishop][antidiag_pattern];
}

bitboard get_queen_attacks(square queen, bitboard all_pieces) {
    return get_bishop_attacks(queen, all_pieces) |
           get_rook_attacks(queen, all_pieces);
}

bitboard generate_attack_map(board_t board, turn side) {
    bitboard attack_map = 0;
    size_t pc_loc;
    bitboard color_pieces;
    bitboard all_pieces_no_king;
    piece curr_piece;
    piece color;
    if(side == W) {
        all_pieces_no_king = board.all_pieces & 
                            ~board.piece_boards[BLACK_KINGS_INDEX];
        color_pieces = board.white_pieces;
        color = WHITE;
    }
    else {
        all_pieces_no_king = board.all_pieces & 
                            ~board.piece_boards[WHITE_KINGS_INDEX];
        color_pieces = board.black_pieces;
        color = BLACK;
    }

    while(color_pieces) {
        pc_loc = first_set_bit(color_pieces);
        curr_piece = board.sq_board[pc_loc];
        if(curr_piece == (color | PAWN)) {
            attack_map |= get_pawn_attacks((square)pc_loc, side);
        }
        else if(curr_piece == (color | KNIGHT)) {
            attack_map |= get_knight_attacks((square)pc_loc);
        }
        else if(curr_piece == (color | BISHOP)) {
            attack_map |= get_bishop_attacks((square)pc_loc, all_pieces_no_king);
        }
        else if(curr_piece == (color | ROOK)) {
            attack_map |= get_rook_attacks((square)pc_loc, all_pieces_no_king);
        }
        else if(curr_piece == (color | QUEEN)) {
            attack_map |= get_queen_attacks((square)pc_loc, all_pieces_no_king);
        }
        else if(curr_piece == (color | KING)) {
            attack_map |= get_king_attacks((square)pc_loc);
        }
        REMOVE_FIRST(color_pieces);
    }
    return attack_map;
}

bitboard get_ray_from_bishop_to_king(square bishop, square king) {
    bitboard board = luts.pieces[bishop] | luts.pieces[king];
    bitboard bishop_attacks_from_bishop = get_bishop_attacks(bishop, board);
    bitboard bishop_attacks_from_king = get_bishop_attacks(king, board);
    return (bishop_attacks_from_bishop & bishop_attacks_from_king) | board;
}

bitboard get_ray_from_rook_to_king(square rook, square king) {
    // assume the board is empty for calculating the rays
    bitboard board = luts.pieces[rook] | luts.pieces[king];
    bitboard rook_attacks_from_rook = get_rook_attacks(rook, board);
    bitboard rook_attacks_from_king = get_rook_attacks(king, board);
    return (rook_attacks_from_rook & rook_attacks_from_king) | board;
}

bitboard get_ray_from_queen_to_king(square queen, square king) {
    if((FILE(queen) == FILE(king)) || (RANK(queen) == RANK(king))) {
        return get_ray_from_rook_to_king(queen, king); // if on the same rank or file, treat the queen as a rook
    }
    return get_ray_from_bishop_to_king(queen, king); // if on the same diagonal, treat the queen as a bishop
}

bitboard get_ray_from_sq_to_sq(square start, square target) {
    return get_ray_from_queen_to_king(start, target);
}

// this function does include the king
piece least_valued_attacker(square sq) {
    bitboard opponent_knights;
    bitboard opponent_kings;
    bitboard opponent_pawns;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    if(b.t == W) {
        opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
        opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
        opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
        opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
    }
    else {
        opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
        opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
        opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
        opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
    }
    if(get_pawn_attacks(sq, b.t) & opponent_pawns) return PAWN;
    if(get_knight_attacks(sq) & opponent_knights) return KNIGHT;
    if(get_bishop_attacks(sq, b.all_pieces) & opponent_bishops) return BISHOP;
    if(get_rook_attacks(sq, b.all_pieces) & opponent_rooks) return ROOK;
    if(get_queen_attacks(sq, b.all_pieces) & opponent_queens) return QUEEN;
    if(get_king_attacks(sq) & opponent_kings) return QUEEN;
    return EMPTY;
}

bool is_attacked_by_pawn(square sq) {
    bitboard opponent_pawns;
    if(b.t == W)
        opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
    else
        opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
    if(get_pawn_attacks(sq, b.t) & opponent_pawns) return true;
    return false;
}

bool is_attacked(square sq, bitboard blocking_pieces) {
    bitboard opponent_knights;
    bitboard opponent_kings;
    bitboard opponent_pawns;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    if(b.t == W) {
        opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
        opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
        opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
        opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
    }
    else {
        opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
        opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
        opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
        opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
    }
    if(get_bishop_attacks(sq, blocking_pieces) & opponent_bishops) return true;
    if(get_rook_attacks(sq, blocking_pieces) & opponent_rooks) return true;
    if(get_knight_attacks(sq) & opponent_knights) return true;
    if(get_pawn_attacks(sq, b.t) & opponent_pawns) return true;
    if(get_queen_attacks(sq, blocking_pieces) & opponent_queens) return true;
    if(get_king_attacks(sq) & opponent_kings) return true;
    return false;
}

bitboard attackers_from_square(square sq) {
    bitboard attackers = 0;
    bitboard opponent_knights;
    bitboard opponent_kings;
    bitboard opponent_pawns;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    if(b.t == W) {
        opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
        opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
        opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
        opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
    }
    else {
        opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
        opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
        opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
        opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
    }
    attackers |= get_knight_attacks(sq) & opponent_knights;
    attackers |= get_king_attacks(sq) & opponent_kings;
    attackers |= get_pawn_attacks(sq, b.t) & opponent_pawns;
    attackers |= get_rook_attacks(sq, b.all_pieces) & opponent_rooks;
    attackers |= get_bishop_attacks(sq, b.all_pieces) & opponent_bishops;
    attackers |= get_queen_attacks(sq, b.all_pieces) & opponent_queens;
    return attackers;
}

bitboard opponent_slider_rays_to_square(square sq) {
    bitboard res = 0;
    square attacker_loc;
    bitboard attackers;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    if(b.t == W) {
        opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
    }
    else {
        opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
    }
    bitboard rook_attacks_from_sq = get_rook_attacks(sq, b.all_pieces);
    attackers = rook_attacks_from_sq & (opponent_rooks | opponent_queens);
    while(attackers) {
        attacker_loc = (square)first_set_bit(attackers);
        res |= get_rook_attacks(attacker_loc, b.all_pieces) & rook_attacks_from_sq;
        REMOVE_FIRST(attackers);    
    }
    bitboard bishop_attacks_from_sq = get_bishop_attacks(sq, b.all_pieces);
    attackers = bishop_attacks_from_sq & (opponent_bishops | opponent_queens);
    while(attackers) {
        attacker_loc = (square)first_set_bit(attackers);
        res |= get_bishop_attacks(attacker_loc, b.all_pieces) & bishop_attacks_from_sq;
        REMOVE_FIRST(attackers);
    }
    return res;
}