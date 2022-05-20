#include <cstdio>
#include <iostream>
#include <cmath>
#include <string>
#include <bitset>
#include <vector>
#include <stack>
#include <time.h>

using namespace std;

typedef long long unsigned int bitboard;
typedef short unsigned int uint16_t;
typedef short unsigned int piece;
typedef bool turn;

#define W (turn)true
#define B (turn)false

#define WHITE (piece)0x0
#define BLACK (piece)0x1
#define PAWN (piece)0x2
#define KNIGHT (piece)0x4
#define BISHOP (piece)0x6
#define ROOK (piece)0x8
#define QUEEN (piece)0xA
#define KING (piece)0xC
#define EMPTY (piece)0x0

#define WHITE_PAWNS_INDEX 0x0
#define BLACK_PAWNS_INDEX 0x1
#define WHITE_KNIGHTS_INDEX 0x2
#define BLACK_KNIGHTS_INDEX 0x3
#define WHITE_BISHOPS_INDEX 0x4
#define BLACK_BISHOPS_INDEX 0x5
#define WHITE_ROOKS_INDEX 0x6
#define BLACK_ROOKS_INDEX 0x7
#define WHITE_QUEENS_INDEX 0x8
#define BLACK_QUEENS_INDEX 0x9
#define WHITE_KINGS_INDEX 0xA
#define BLACK_KINGS_INDEX 0xB

#define NO_CHECK 0x0
#define SINGLE_CHECK 0x1
#define DOUBLE_CHECK 0x2

enum square { A1, B1, C1, D1, E1, F1, G1, H1,
              A2, B2, C2, D2, E2, F2, G2, H2,
              A3, B3, C3, D3, E3, F3, G3, H3,
              A4, B4, C4, D4, E4, F4, G4, H4,
              A5, B5, C5, D5, E5, F5, G5, H5,
              A6, B6, C6, D6, E6, F6, G6, H6,
              A7, B7, C7, D7, E7, F7, G7, H7,
              A8, B8, C8, D8, E8, F8, G8, H8, NONE };

enum rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
enum file { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

typedef struct Board
{
    bitboard piece_boards[12];

    bitboard white_pieces;
    bitboard black_pieces;
    bitboard all_pieces;

    piece sq_board[64];

    turn t;
    square en_passant;
    bool white_king_side;
    bool white_queen_side;
    bool black_king_side;
    bool black_queen_side;

    bitboard white_attack_map;
    bitboard black_attack_map;

    square white_king_loc;
    square black_king_loc;
} board_t;

typedef struct LUTs {
    bitboard clear_rank[8];
    bitboard mask_rank[8];
    bitboard clear_file[8];
    bitboard mask_file[8];
    bitboard mask_diagonal[15]; // only 15 diagonals, 8th is for alignment
    bitboard mask_antidiagonal[15];
    bitboard pieces[64];

    bitboard king_attacks[64]; // these are currently unused
    bitboard white_pawn_attacks[64];
    bitboard black_pawn_attacks[64];
    bitboard white_pawn_pushes[64];
    bitboard black_pawn_pushes[64];
    bitboard knight_attacks[64];

    bitboard rank_attacks[64][256]; 
    bitboard file_attacks[64][256];
    bitboard diagonal_attacks[64][256]; // a1 to h8 diagonal
    bitboard antidiagonal_attacks[64][256]; // a8 to h1 diagonal
} lut_t;

/* 
   Might be worth having the move struct hold info like is_en_passant, 
   captured piece, and moving piece.
*/
typedef struct move_struct {
    square start;
    square target;
    piece mv_piece;
    piece tar_piece;
    piece promotion_piece;
} move_t;

typedef struct pin_struct {
    bitboard ray_at_sq[64];
    bitboard pinned_pieces;
} pin_t;

int capture_moves = 0;
int en_passant_moves = 0;
int castle_moves = 0;
int promotion_moves = 0;
int checks = 0;
int double_checks = 0;


// stolen from chessprogramming wiki
bitboard flip_vertical(bitboard b) {
    return  ((b << 56)                          ) |
            ((b << 40) & bitboard(0x00ff000000000000)) |
            ((b << 24) & bitboard(0x0000ff0000000000)) |
            ((b <<  8) & bitboard(0x000000ff00000000)) |
            ((b >>  8) & bitboard(0x00000000ff000000)) |
            ((b >> 24) & bitboard(0x0000000000ff0000)) |
            ((b >> 40) & bitboard(0x000000000000ff00)) |
            ((b >> 56));
}
// stolen from chessprogramming wiki
bitboard flip_diag_a1_h8(bitboard b) {
    bitboard t;
    const bitboard k1 = bitboard(0x5500550055005500);
    const bitboard k2 = bitboard(0x3333000033330000);
    const bitboard k4 = bitboard(0x0F0F0F0F00000000);
    t = k4 & (b ^ (b << 28));
    b ^= t ^ (t >> 28);
    t = k2 & (b ^ (b << 14));
    b ^= t ^ (t >> 14);
    t = k1 & (b ^ (b << 7));
    b ^= t ^ (t >> 7);
    return b;
}

bitboard rotate_90_anticlockwise(bitboard b) {
    return flip_diag_a1_h8(flip_vertical(b));
}

bitboard rotate_90_clockwise(bitboard b) {
    return flip_vertical(flip_diag_a1_h8(b));
}

/*
 * Like bit shifting but it wraps around the number
 * Also stolen from chessprogramming wiki
*/
bitboard rotate_left(bitboard b, int s) {return (b << s) | (b >> (64 - s));}
bitboard rotate_right(bitboard b, int s) {return (b >> s) | (b << (64 - s));}

/* 
 * Rotates the board by 45 degrees clockwise
 * Stolen from chessprogramming wiki 
*/
bitboard pseudo_rotate_45_clockwise(bitboard b) {
    const bitboard k1 = bitboard(0xAAAAAAAAAAAAAAAA);
    const bitboard k2 = bitboard(0xCCCCCCCCCCCCCCCC);
    const bitboard k4 = bitboard(0xF0F0F0F0F0F0F0F0);
    
    b ^= k1 & (b ^ rotate_right(b, 8));
    b ^= k2 & (b ^ rotate_right(b, 16));
    b ^= k4 & (b ^ rotate_right(b, 32));
    return b;
}

bitboard undo_pseudo_rotate_45_clockwise(bitboard b) {
    for (size_t i = 0; i < 7; i++) {
        b = pseudo_rotate_45_clockwise(b);
    }
    return b;
}

/* 
 * Rotates the board by 45 degrees anti-clockwise
 * Stolen from chessprogramming wiki 
*/
bitboard pseudo_rotate_45_anticlockwise(bitboard b) {
    const bitboard k1 = bitboard(0x5555555555555555);
    const bitboard k2 = bitboard(0x3333333333333333);
    const bitboard k4 = bitboard(0x0F0F0F0F0F0F0F0F);
    
    b ^= k1 & (b ^ rotate_right(b, 8));
    b ^= k2 & (b ^ rotate_right(b, 16));
    b ^= k4 & (b ^ rotate_right(b, 32));
    return b;
}

bitboard undo_pseudo_rotate_45_anticlockwise(bitboard b) {
    for (size_t i = 0; i < 7; i++) {
        b = pseudo_rotate_45_anticlockwise(b);
    }
    return b;
}

lut_t *init_LUT () {
    lut_t *luts = (lut_t *) malloc(sizeof(lut_t));

    bitboard rank = 0x00000000000000FF;
    bitboard file = 0x0101010101010101;
    /* creating rank and file masks and clears */
    for(size_t i = 0; i < 8; i++) {
        luts->mask_rank[i] = rank;
        luts->clear_rank[i] = ~rank;
        luts->mask_file[i] = file;
        luts->clear_file[i] = ~file;

        rank = rank << 8;
        file = file << 1;
    }

    /* creating diagonal masks */
    /* a diagonal is identified by file - rank if file >= rank else 15 - (rank - file) */
    luts->mask_diagonal[0] = 0x8040201008040201;
    luts->mask_diagonal[1] = 0x0080402010080402;
    luts->mask_diagonal[2] = 0x0000804020100804;
    luts->mask_diagonal[3] = 0x0000008040201008;
    luts->mask_diagonal[4] = 0x0000000080402010;
    luts->mask_diagonal[5] = 0x0000000000804020;
    luts->mask_diagonal[6] = 0x0000000000008040;
    luts->mask_diagonal[7] = 0x0000000000000080;
    luts->mask_diagonal[8] = 0x0100000000000000;
    luts->mask_diagonal[9] = 0x0201000000000000;
    luts->mask_diagonal[10] = 0x0402010000000000;
    luts->mask_diagonal[11] = 0x0804020100000000;
    luts->mask_diagonal[12] = 0x1008040201000000;
    luts->mask_diagonal[13] = 0x2010080402010000;
    luts->mask_diagonal[14] = 0x4020100804020100;

    /* creating antidiagonal masks */
    /* an antidiagonal is identified by (7 - file) - rank if file >= rank else rank - (7 - file) + 1*/
    luts->mask_antidiagonal[0] = 0x0102040810204080;
    luts->mask_antidiagonal[1] = 0x0001020408102040;
    luts->mask_antidiagonal[2] = 0x0000010204081020;
    luts->mask_antidiagonal[3] = 0x0000000102040810; 
    luts->mask_antidiagonal[4] = 0x0000000001020408;
    luts->mask_antidiagonal[5] = 0x0000000000010204;
    luts->mask_antidiagonal[6] = 0x0000000000000102;
    luts->mask_antidiagonal[7] = 0x0000000000000001;
    luts->mask_antidiagonal[8] = 0x8000000000000000;
    luts->mask_antidiagonal[9] = 0x4080000000000000;
    luts->mask_antidiagonal[10] = 0x2040800000000000;
    luts->mask_antidiagonal[11] = 0x1020408000000000;
    luts->mask_antidiagonal[12] = 0x0810204080000000;
    luts->mask_antidiagonal[13] = 0x0408102040800000;
    luts->mask_antidiagonal[14] = 0x0204081020408000;

    /* creating piece masks */
    bitboard piece = 0x0000000000000001;
    for(size_t i = 0; i < 64; i++) {
        luts->pieces[i] = piece;
        piece = piece << 1;
    }

    /* creating knight_attacks LUT */
    bitboard spot_1_clip = luts->clear_file[FILE_H] & luts->clear_file[FILE_G];
    bitboard spot_2_clip = luts->clear_file[FILE_H];
    bitboard spot_3_clip = luts->clear_file[FILE_A];
    bitboard spot_4_clip = luts->clear_file[FILE_A] & luts->clear_file[FILE_B];

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
        knight = luts->pieces[sq];
        spot_1 = (knight & spot_1_clip) >> 6;
        spot_2 = (knight & spot_2_clip) >> 15;
        spot_3 = (knight & spot_3_clip) >> 17;
        spot_4 = (knight & spot_4_clip) >> 10;

        spot_5 = (knight & spot_5_clip) << 6;
        spot_6 = (knight & spot_6_clip) << 15;
        spot_7 = (knight & spot_7_clip) << 17;
        spot_8 = (knight & spot_8_clip) << 10;
        luts->knight_attacks[sq] = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                                   spot_7 | spot_8;
    }

    /* creating king_attacks LUT */
    spot_1_clip = luts->clear_file[FILE_A];
    spot_3_clip = luts->clear_file[FILE_H];
    spot_4_clip = luts->clear_file[FILE_H];

    spot_5_clip = luts->clear_file[FILE_H];
    spot_7_clip = luts->clear_file[FILE_A];
    spot_8_clip = luts->clear_file[FILE_A];

    bitboard king;
    for(size_t sq = 0; sq < 64; sq++) {
        king = luts->pieces[sq];
        spot_1 = (king & spot_1_clip) << 7;
        spot_2 = king << 8;
        spot_3 = (king & spot_3_clip) << 9;
        spot_4 = (king & spot_4_clip) << 1;

        spot_5 = (king & spot_5_clip) >> 7;
        spot_6 = king >> 8;
        spot_7 = (king & spot_7_clip) >> 9;
        spot_8 = (king & spot_8_clip) >> 1;
        luts->king_attacks[sq] = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                                 spot_7 | spot_8;
    }

    /* creating white_pawn_attacks LUT */
    bitboard pawn;
    spot_1_clip = luts->clear_file[FILE_A];
    spot_4_clip = luts->clear_file[FILE_H];
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts->pieces[sq];
        spot_1 = (pawn & spot_1_clip) << 7;
        spot_4 = (pawn & spot_4_clip) << 9;
        luts->white_pawn_attacks[sq] = spot_1 | spot_4;
    }

    /* creating black_pawn_attacks LUT */
    spot_1_clip = luts->clear_file[FILE_A];
    spot_4_clip = luts->clear_file[FILE_H];
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts->pieces[sq];
        spot_1 = (pawn & spot_4_clip) >> 7;
        spot_4 = (pawn & spot_1_clip) >> 9;
        luts->black_pawn_attacks[sq] = spot_1 | spot_4;
    }

    /* 
        creating white_pawn_pushes LUT 
        does not include double pawn pushes for 2nd rank pawns
        that calculation is done in the generate_pawn_moves function
    */
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts->pieces[sq];
        spot_2 = pawn << 8;
        luts->white_pawn_pushes[sq] = spot_2;
    }

    /* 
        creating black_pawn_pushes LUT 
        does not include double pawn pushes for 7th rank pawns
        that calculation is done in the generate_pawn_moves function
    */
    for(size_t sq = 0; sq < 64; sq++) {
        pawn = luts->pieces[sq];
        spot_2 = pawn >> 8;
        luts->black_pawn_pushes[sq] = spot_2;
    }

    /* creating rank_attacks LUT */  
    size_t LSB_to_first_1;
    size_t LSB_to_second_1;
    bitboard mask_1 = 0x1;
    bitboard rank_attack_mask = 0xFF; // shift this at end to get attacks
    for(size_t sq = 0; sq < 64; sq++) {
        size_t file = sq % 8; // will have to multiply later to shift the bitboard into the right spot
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
            luts->rank_attacks[sq][pattern] = (unplaced_mask << (sq - (file - LSB_to_first_1))) 
                                              & ~luts->pieces[sq]; // removes piece square
        }
    }

    /* creating file_attacks LUT */
    for(size_t i = 0; i < 8; i++) {
        for(size_t j = 0; j < 8; j++){
            for(size_t pattern = 0; pattern < 256; pattern++) {
                luts->file_attacks[i*8 + j][pattern] = rotate_90_clockwise(luts->rank_attacks[j*8 + (7-i)][pattern]);
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
                luts->diagonal_attacks[sq][pattern] = 
                    undo_pseudo_rotate_45_clockwise(luts->rank_attacks[rotated_sq][pattern])
                    & luts->mask_diagonal[diag] & ~luts->pieces[sq]; // removes piece square
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
                luts->antidiagonal_attacks[sq][pattern] = 
                    undo_pseudo_rotate_45_anticlockwise(luts->rank_attacks[rotated_sq][pattern])
                    & luts->mask_antidiagonal[diag] & ~luts->pieces[sq]; // removes piece square
            }
        }
        
    }
    return luts;
}

size_t bitboard_index_from_piece(piece pc) {
    return pc - 2; //this is to offset the existence of the two EMPTY (0000 and 0001)
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
    board.white_king_side = true;
    board.white_queen_side = true;
    board.black_king_side = true;
    board.black_queen_side = true;

    board.white_attack_map = 0;
    board.black_attack_map = 0;
    return board;
}

/* returns the 0-indexed location of the first 1 bit from LSB to MSB
   also returns 0 when the input is 0 (so case on that) */
uint16_t first_set_bit(bitboard bits) {
    return log2(bits & -bits);
}

bitboard rem_first_bit(bitboard bits) {
    return bits & (bits - 1);
}

/* returns the 0-indexed location of the first 1 bit from MSB to LSB
   also returns 0 when the input is 0 (so case on that) */
uint16_t last_set_bit(bitboard bits) {
    uint16_t count = 0;
    while (bits > 0) {
        bits = bits >> 1;
        count++;
    }
    return count;
}

bitboard get_knight_attacks(square knight, lut_t *luts) {
    // doesn't depend on the current position
    return luts->knight_attacks[knight];
}

bitboard get_king_attacks(square king, lut_t *luts) {
    // doesn't depend on the current position
    return luts->king_attacks[king];
}

bitboard get_pawn_attacks(square pawn, turn side, lut_t *luts) {
    // depends on who's turn it is to move
    if(side == W) return luts->white_pawn_attacks[pawn];
    return luts->black_pawn_attacks[pawn];
}

bitboard get_rook_attacks(square rook, bitboard all_pieces, lut_t *luts) {
    // depends on the placement of all_pieces on the board
    // could leave out the white king when calculating blacks attack maps
    size_t rook_rank = (size_t)rook / 8;
    size_t rook_file = (size_t)rook % 8;
    bitboard rank_pattern = (all_pieces & luts->mask_rank[rook_rank]) >> (rook_rank * 8);
    bitboard file_pattern = rotate_90_anticlockwise(all_pieces & luts->mask_file[rook_file]) >> (rook_file * 8);
    return luts->rank_attacks[rook][rank_pattern] |
           luts->file_attacks[rook][file_pattern];
}

bitboard get_bishop_attacks(square bishop, bitboard all_pieces, lut_t *luts) {
    // depends on the placement of all_pieces on the board
    // could leave out the white king when calculating blacks attack maps
    // this code is literal dog shit please clean it up later jason...
    size_t bishop_rank = (size_t)bishop / 8;
    size_t bishop_file = (size_t)bishop % 8;
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
    bitboard diag_pattern = pseudo_rotate_45_clockwise(all_pieces & luts->mask_diagonal[bishop_diag]) >> (8 * rotated_rank);
    size_t antirotated_rank;
    if(bishop_antidiag == 0)      antirotated_rank = 0;
    else if(bishop_antidiag < 8)  antirotated_rank = 8 - bishop_antidiag;
    else                          antirotated_rank = 8 - (bishop_antidiag - 7);
    bitboard antidiag_pattern = pseudo_rotate_45_anticlockwise(all_pieces & luts->mask_antidiagonal[bishop_antidiag]) >> (8 * antirotated_rank);
    return luts->diagonal_attacks[bishop][diag_pattern] |
           luts->antidiagonal_attacks[bishop][antidiag_pattern];
}

bitboard get_queen_attacks(square queen, bitboard all_pieces, lut_t *luts) {
    return get_bishop_attacks(queen, all_pieces, luts) |
           get_rook_attacks(queen, all_pieces, luts);
}

bitboard generate_attack_map(board_t board, turn side, lut_t *luts) {
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
            attack_map |= get_pawn_attacks((square)pc_loc, side, luts);
        }
        else if(curr_piece == (color | KNIGHT)) {
            attack_map |= get_knight_attacks((square)pc_loc, luts);
        }
        else if(curr_piece == (color | BISHOP)) {
            attack_map |= get_bishop_attacks((square)pc_loc, all_pieces_no_king, luts);
        }
        else if(curr_piece == (color | ROOK)) {
            attack_map |= get_rook_attacks((square)pc_loc, all_pieces_no_king, luts);
        }
        else if(curr_piece == (color | QUEEN)) {
            attack_map |= get_queen_attacks((square)pc_loc, all_pieces_no_king, luts);
        }
        else if(curr_piece == (color | KING)) {
            attack_map |= get_king_attacks((square)pc_loc, luts);
        }
        color_pieces = rem_first_bit(color_pieces);
    }
    return attack_map;
}

board_t update_boards(board_t board, lut_t *luts) {
    board.black_pieces = (board.piece_boards[1]  | board.piece_boards[3] | 
                           board.piece_boards[5]  | board.piece_boards[7] |
                           board.piece_boards[9]  | board.piece_boards[11]);

    board.white_pieces = (board.piece_boards[0]  | board.piece_boards[2] | 
                           board.piece_boards[4]  | board.piece_boards[6] |
                           board.piece_boards[8]  | board.piece_boards[10]);

    board.all_pieces = board.white_pieces | board.black_pieces;
    if(board.t == W) {
        // generate the opponents attack map for king moves
        board.black_attack_map = generate_attack_map(board, B, luts);
    }
    else {
        board.white_attack_map = generate_attack_map(board, W, luts);
    }
    return board;
}

bool is_sliding_piece(piece pc) {
    piece masked_pc = pc & 0xE;
    if(masked_pc == ROOK || masked_pc == BISHOP || masked_pc == QUEEN) return true;
    return false;
}

bitboard place_piece(bitboard board, square sq, lut_t *luts) {
    return board | luts->pieces[sq];
}

bitboard rem_piece(bitboard board, square sq, lut_t *luts) {
    return board & ~(luts->pieces[sq]);
}

board_t make_move(board_t board, move_t move, lut_t *luts) {
    square start = move.start;
    square target = move.target;

    piece mv_piece = move.mv_piece;
    piece tar_piece = move.tar_piece;

    bitboard *mv_board = &board.piece_boards[bitboard_index_from_piece(mv_piece)];
    
    // always remove the piece from its board no matter what
    *mv_board = rem_piece(*mv_board, start, luts);

    /* update king locations */
    if (mv_piece == (WHITE | KING)) board.white_king_loc = target;
    else if (mv_piece == (BLACK | KING)) board.black_king_loc = target;

    /* check for castling move */
    bitboard *castling_rook;
    if(mv_piece == (WHITE | KING) && (start == E1) && (target == G1)) {
        castling_rook = &board.piece_boards[WHITE_ROOKS_INDEX];
        *castling_rook = rem_piece(*castling_rook, H1, luts);
        *castling_rook = place_piece(*castling_rook, F1, luts);
        board.sq_board[H1] = EMPTY;
        board.sq_board[F1] = WHITE | ROOK;
        castle_moves++; // DEBUGGING
    }
    else if(mv_piece == (WHITE | KING) && (start == E1) && (target == C1)) {
        castling_rook = &board.piece_boards[WHITE_ROOKS_INDEX];
        *castling_rook = rem_piece(*castling_rook, A1, luts);
        *castling_rook = place_piece(*castling_rook, D1, luts);
        board.sq_board[A1] = EMPTY;
        board.sq_board[D1] = WHITE | ROOK;
        castle_moves++; // DEBUGGING
    }
    else if(mv_piece == (BLACK | KING) && (start == E8) && (target == G8)) {
        castling_rook = &board.piece_boards[BLACK_ROOKS_INDEX];
        *castling_rook = rem_piece(*castling_rook, H8, luts);
        *castling_rook = place_piece(*castling_rook, F8, luts);
        board.sq_board[H8] = EMPTY;
        board.sq_board[F8] = BLACK | ROOK;
        castle_moves++; // DEBUGGING
    }
    else if(mv_piece == (BLACK | KING) && (start == E8) && (target == C8)) {
        castling_rook = &board.piece_boards[BLACK_ROOKS_INDEX];
        *castling_rook = rem_piece(*castling_rook, A8, luts);
        *castling_rook = place_piece(*castling_rook, D8, luts);
        board.sq_board[A8] = EMPTY;
        board.sq_board[D8] = BLACK | ROOK;
        castle_moves++; // DEBUGGING
    }

    /* check if promoting move */
    if(move.promotion_piece != EMPTY) {
        mv_piece = move.promotion_piece;
        mv_board = &board.piece_boards[bitboard_index_from_piece(mv_piece)];
        promotion_moves++; // DEBUGGING
    }

    *mv_board = place_piece(*mv_board, target, luts);

    if(tar_piece != EMPTY) {
        bitboard *captured_board = &board.piece_boards[bitboard_index_from_piece(tar_piece)];
        *captured_board = rem_piece(*captured_board, target, luts);
        capture_moves++; // DEBUGGING
    }

    board.sq_board[start] = EMPTY;
    board.sq_board[target] = mv_piece; // mv_piece will be updated to queen if promoting move

    /* check for en passant move to remove the pawn being captured en passant */
    if(mv_piece == (WHITE | PAWN) && target == board.en_passant) {
        bitboard *black_pawns = &board.piece_boards[BLACK_PAWNS_INDEX];
        square pawn_square = (square)((int)board.en_passant - 8);
        *black_pawns = rem_piece(*black_pawns, pawn_square, luts);
        board.sq_board[pawn_square] = EMPTY;
        en_passant_moves++; // DEBUGGING
    }
    else if(mv_piece == (BLACK | PAWN) && target == board.en_passant) {
        bitboard *white_pawns = &board.piece_boards[WHITE_PAWNS_INDEX];
        square pawn_square = (square)((int)board.en_passant + 8);
        *white_pawns = rem_piece(*white_pawns, pawn_square, luts);
        board.sq_board[pawn_square] = EMPTY;
        en_passant_moves++; // DEBUGGING
    }

    /* Update en passant squares */
    if(mv_piece == (WHITE | PAWN) && target - start == 16) {
        board.en_passant = (square)(start + 8);
    }
    else if(mv_piece == (BLACK | PAWN) && start - target == 16) {
        board.en_passant = (square)(target + 8);
    }
    else {
        board.en_passant = NONE;
    }

    /* Update castling rights for king moves */
    if(mv_piece == (WHITE | KING)) {
        board.white_king_side = false;
        board.white_queen_side = false;
    }
    else if(mv_piece == (BLACK | KING)) {
        board.black_king_side = false;
        board.black_queen_side = false;
    }
    /* Update castling rights for rook moves */
    else if(mv_piece == (WHITE | ROOK) && start == H1) {
        board.white_king_side = false;
    }
    else if(mv_piece == (WHITE | ROOK) && start == A1) {
        board.white_queen_side = false;
    }
    else if(mv_piece == (BLACK | ROOK) && start == H8) {
        board.black_king_side = false;
    }
    else if(mv_piece == (BLACK | ROOK) && start == A8) {
        board.black_queen_side = false;
    }

    board.t = !board.t;
    return update_boards(board, luts);
}

stack<board_t> unmake_move(stack<board_t> boards) {
    boards.pop();
    return boards;
}

board_t decode_fen(string fen, lut_t *luts) {
    board_t board = zero_board();
    bitboard *place_board;
    piece pc;
    int col = 0;
    int row = 7;
    int loc;
    for (size_t i = 0; i < fen.size(); i++) {
        char c = fen[i];
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
            
            board.sq_board[loc] = pc;
            place_board = &board.piece_boards[bitboard_index_from_piece(pc)];
            *place_board = place_piece(*place_board, (square)loc, luts);
            col += 1;
        }

    }
    return update_boards(board, luts);
}

void print_bitboard (bitboard b) {
    bitset<64> bs(b);
    for (size_t i = 0; i < 8; i++) {
        for (size_t j = 0; j < 8; j++) {
            cout << bs[(7-i)*8 + j];
        }
        cout << endl;
    }
}

void print_squarewise(piece sqs[64]) {
    char c;
    piece pc;
    for (size_t i = 0; i < 8; i++) {
        for (size_t j = 0; j < 8; j++) {
            pc = sqs[(7-i)*8 + j];
            switch (pc) {
                case (WHITE | PAWN) :
                    c = 'P';
                    break;
                case (BLACK | PAWN) :
                    c = 'p';
                    break;
                case (WHITE | KNIGHT) :
                    c = 'N';
                    break;
                case (BLACK | KNIGHT) :
                    c = 'n';
                    break;
                case (WHITE | BISHOP) :
                    c = 'B';
                    break;
                case (BLACK | BISHOP) :
                    c = 'b';
                    break;
                case (WHITE | ROOK) :
                    c = 'R';
                    break;
                case (BLACK | ROOK) :
                    c = 'r';
                    break;
                case (WHITE | QUEEN) :
                    c = 'Q';
                    break;
                case (BLACK | QUEEN) :
                    c = 'q';
                    break;
                case (WHITE | KING) :
                    c = 'K';
                    break;
                case (BLACK | KING) :
                    c = 'k';
                    break;
                default:
                    c = '*';
                    break;
            }
            cout << c;
        }
        cout << endl;
    }
    cout << endl;
}

void print_board(board_t board) {
    bitboard all_pieces = (board.all_pieces);
    bitboard white_pieces = (board.white_pieces);
    bitboard black_pieces = (board.black_pieces);

    bitboard white_pawns = (board.piece_boards[0]);
    bitboard black_pawns = (board.piece_boards[1]);
    bitboard white_knights = (board.piece_boards[2]);
    bitboard black_knights = (board.piece_boards[3]);
    bitboard white_bishops = (board.piece_boards[4]);
    bitboard black_bishops = (board.piece_boards[5]);
    bitboard white_rooks = (board.piece_boards[6]);
    bitboard black_rooks = (board.piece_boards[7]);
    bitboard white_queens = (board.piece_boards[8]);
    bitboard black_queens = (board.piece_boards[9]);
    bitboard white_kings = (board.piece_boards[10]);
    bitboard black_kings = (board.piece_boards[11]);

    cout << "All Pieces" << endl;
    print_bitboard(all_pieces);

    cout << endl << "White Pieces" << endl;
    print_bitboard(white_pieces);

    cout << endl <<  "Black Pieces" << endl;
    print_bitboard(black_pieces);

    cout << endl <<  "White Pawns" << endl;
    print_bitboard(white_pawns);

    cout << endl <<  "Black Pawns" << endl;
    print_bitboard(black_pawns);

    cout << endl <<  "White Knights" << endl;
    print_bitboard(white_knights);

    cout << endl <<  "Black Knights" << endl;
    print_bitboard(black_knights);

    cout << endl <<  "White Bishops" << endl;
    print_bitboard(white_bishops);

    cout << endl <<  "Black Bishops" << endl;
    print_bitboard(black_bishops);

    cout << endl <<  "White Rooks" << endl;
    print_bitboard(white_rooks);

    cout << endl <<  "Black Rooks" << endl;
    print_bitboard(black_rooks);

    cout << endl <<  "White Queens" << endl;
    print_bitboard(white_queens);

    cout << endl <<  "Black Queens" << endl;
    print_bitboard(black_queens);

    cout << endl <<  "White Kings" << endl;
    print_bitboard(white_kings);

    cout << endl <<  "Black Kings" << endl;
    print_bitboard(black_kings);

    cout << endl <<  "Square-wise" << endl;
    print_squarewise(board.sq_board);
    return;
}

bitboard generate_knight_move_bitboard(square knight, board_t board, lut_t *luts) {
    bitboard own_pieces;
    if(board.t == W)  own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;
    
    bitboard knight_attacks = luts->knight_attacks[knight];

    return knight_attacks & ~own_pieces;
}

bitboard generate_king_move_bitboard(square king, board_t board, lut_t *luts) {
    // still need to add castling... will have to take in info from the board for that
    bitboard own_pieces;
    bitboard attacked_squares;
    if(board.t == W)  {
        own_pieces = board.white_pieces; 
        attacked_squares = board.black_attack_map;
    }
    else {
        own_pieces = board.black_pieces; 
        attacked_squares = board.white_attack_map;
    }
    
    bitboard king_attacks = luts->king_attacks[king];
    bitboard king_pseudomoves = king_attacks & ~own_pieces;

    if(!king_pseudomoves) return 0;

    // generate castling moves
    // maybe move these to defined constants
    const bitboard w_king_side_pieces = 0x60;
    const bitboard w_queen_side_pieces = 0xE;
    const bitboard b_king_side_pieces = 0x6000000000000000;
    const bitboard b_queen_side_pieces = 0x0E00000000000000;
    const bitboard w_king_side_castle = 0x40;
    const bitboard w_queen_side_castle = 0x4;
    const bitboard b_king_side_castle = 0x4000000000000000;
    const bitboard b_queen_side_castle = 0x0400000000000000;
    const bitboard w_king_side_attack = 0x70;
    const bitboard w_queen_side_attack = 0x1C;
    const bitboard b_king_side_attack = 0x7000000000000000;
    const bitboard b_queen_side_attack = 0x1C00000000000000;
    bitboard king_castle = 0;
    if(board.t == W) {
        if(board.white_king_side && 
          ((board.all_pieces & w_king_side_pieces) == 0) &&
          (board.sq_board[H1] == (WHITE | ROOK)) && 
          ((board.black_attack_map & w_king_side_attack) == 0))
          king_castle |= w_king_side_castle;

        if(board.white_queen_side && 
          ((board.all_pieces & w_queen_side_pieces) == 0) &&
          (board.sq_board[A1] == (WHITE | ROOK)) && 
          ((board.black_attack_map & w_queen_side_attack) == 0))
          king_castle |= w_queen_side_castle;
    }
    else {
        if(board.black_king_side && 
          ((board.all_pieces & b_king_side_pieces) == 0) &&
          (board.sq_board[H8] == (BLACK | ROOK)) && 
          ((board.white_attack_map & b_king_side_attack) == 0))
          king_castle |= b_king_side_castle;

        if(board.black_queen_side && 
          ((board.all_pieces & b_queen_side_pieces) == 0) &&
          (board.sq_board[A8] == (BLACK | ROOK)) &&
          ((board.white_attack_map & b_queen_side_attack) == 0))
          king_castle |= b_queen_side_castle;
    }
    return (king_pseudomoves & ~attacked_squares) | king_castle;           
}

bitboard generate_pawn_move_bitboard(square pawn, board_t board, lut_t *luts) {
    // will need to add en passant later
    bitboard enemy_pieces;
    bitboard all_pieces = board.all_pieces;
    bitboard captures;
    bitboard forward_moves;
    bitboard forward_one;
    bitboard forward_two;
    bitboard en_passant_capture;
    square en_passant_sq = board.en_passant;
    bitboard en_passant_bit = 0; // default it to zero
    size_t rank = pawn / 8;
    
    if(en_passant_sq != NONE) {
        en_passant_bit =  luts->pieces[en_passant_sq]; // used to and with attack pattern
    }

    if(board.t == W) {
        enemy_pieces = board.black_pieces;
        captures = luts->white_pawn_attacks[pawn] & enemy_pieces;
        forward_one = luts->white_pawn_pushes[pawn] & ~all_pieces;
        forward_two = 0;
        if(rank == RANK_2 && forward_one) {
            forward_two = luts->white_pawn_pushes[pawn + 8] & ~all_pieces;
        }
        forward_moves = forward_one | forward_two;
        en_passant_capture = luts->white_pawn_attacks[pawn] & en_passant_bit;
    }
    else {
        enemy_pieces = board.white_pieces;
        captures = luts->black_pawn_attacks[pawn] & enemy_pieces;
        forward_one = luts->black_pawn_pushes[pawn] & ~all_pieces;
        forward_two = 0;
        if(rank == RANK_7 && forward_one) {
            forward_two = luts->black_pawn_pushes[pawn - 8] & ~all_pieces;
        }
        forward_moves = forward_one | forward_two;
        en_passant_capture = luts->black_pawn_attacks[pawn] & en_passant_bit;
    }

    return captures | forward_moves | en_passant_capture;
}

bitboard generate_rook_move_bitboard(square rook, board_t board, lut_t *luts) {
    bitboard all_pieces = board.all_pieces;
    bitboard own_pieces;
    if(board.t == W)  own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;

    bitboard rook_attacks = get_rook_attacks(rook, all_pieces, luts);
    return rook_attacks & ~own_pieces;
}

bitboard generate_bishop_move_bitboard(square bishop, board_t board, lut_t *luts) {
    bitboard all_pieces = board.all_pieces;
    bitboard own_pieces;

    if(board.t == W)  own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;

    bitboard bishop_attacks = get_bishop_attacks(bishop, all_pieces, luts);
    
    return  bishop_attacks & ~own_pieces;
}

bitboard generate_queen_move_bitboard(square queen, board_t board, lut_t *luts) {
    return   generate_rook_move_bitboard(queen, board, luts)
           | generate_bishop_move_bitboard(queen, board, luts);
}

bitboard get_ray_from_bishop_to_king(square bishop, square king, lut_t *luts) {
    bitboard board = luts->pieces[bishop] | luts->pieces[king];
    bitboard bishop_attacks_from_bishop = get_bishop_attacks(bishop, board, luts);
    bitboard bishop_attacks_from_king = get_bishop_attacks(king, board, luts);
    return (bishop_attacks_from_bishop & bishop_attacks_from_king) | board;
}

bitboard get_ray_from_rook_to_king(square rook, square king, lut_t *luts) {
    // assume the board is empty for calculating the rays
    bitboard board = luts->pieces[rook] | luts->pieces[king];
    bitboard rook_attacks_from_rook = get_rook_attacks(rook, board, luts);
    bitboard rook_attacks_from_king = get_rook_attacks(king, board, luts);
    return (rook_attacks_from_rook & rook_attacks_from_king) | board;
}

bitboard get_ray_from_queen_to_king(square queen, square king, lut_t *luts) {
    if((queen % 8 == king % 8) || (queen / 8 == king / 8)) {
        return get_ray_from_rook_to_king(queen, king, luts); // if on the same rank or file, treat the queen as a rook
    }
    return get_ray_from_bishop_to_king(queen, king, luts); // if on the same diagonal, treat the queen as a bishop
}

bitboard get_ray_from_sq_to_sq(square start, square target, lut_t *luts) {
    return get_ray_from_queen_to_king(start, target, luts);
}

vector<move_t> generate_knight_moves(board_t board, vector<move_t> curr_moves, bitboard check_mask, pin_t pin, lut_t *luts) {
    move_t move;
    uint16_t pc_loc;
    uint16_t tar_loc;
    move.promotion_piece = EMPTY;
    bitboard knights;       
    piece color;
    if (board.t == W) {
        knights = board.piece_boards[WHITE_KNIGHTS_INDEX];
        color = WHITE;
    }
    else {
        knights = board.piece_boards[BLACK_KNIGHTS_INDEX];
        color = BLACK;
    }
    bitboard knight_moves;
    bitboard knight_bit;
    while(knights) {
        pc_loc = first_set_bit(knights);
        knight_bit = luts->pieces[pc_loc];
        if(knight_bit & pin.pinned_pieces) {
            knights = rem_first_bit(knights);
            continue;
        } // pinned knights cannot move at all
        knight_moves = generate_knight_move_bitboard((square)pc_loc, board, luts) & check_mask;
        while(knight_moves) {
            tar_loc = first_set_bit(knight_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | KNIGHT;
            move.tar_piece = board.sq_board[tar_loc];
            curr_moves.push_back(move);
            knight_moves = rem_first_bit(knight_moves);
        }
        knights = rem_first_bit(knights);
    }
    return curr_moves;
}

vector<move_t> generate_king_moves(board_t board, vector<move_t> curr_moves, lut_t *luts) {
    move_t move;
    uint16_t pc_loc;
    uint16_t tar_loc;
    move.promotion_piece = EMPTY;
    bitboard kings;       
    piece color;
    if (board.t == W) {
        kings = board.piece_boards[WHITE_KINGS_INDEX];
        color = WHITE;
    }
    else {
        kings = board.piece_boards[BLACK_KINGS_INDEX];
        color = BLACK;
    }
    bitboard king_moves;
    while(kings) {
        pc_loc = first_set_bit(kings);
        king_moves = generate_king_move_bitboard((square)pc_loc, board, luts);
        
        while(king_moves) {
            tar_loc = first_set_bit(king_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | KING;
            move.tar_piece = board.sq_board[tar_loc];
            curr_moves.push_back(move);
            king_moves = rem_first_bit(king_moves);
        }
        kings = rem_first_bit(kings);
    }
    return curr_moves;
}

vector<move_t> generate_pawn_moves(board_t board, vector<move_t> curr_moves, bitboard check_mask, pin_t pin, lut_t *luts) {
    move_t move;
    uint16_t pc_loc;
    uint16_t tar_loc;
    move.promotion_piece = EMPTY;
    bitboard pin_mask;
    bitboard pawns;       
    piece color;
    if (board.t == W) {
        pawns = board.piece_boards[WHITE_PAWNS_INDEX];
        color = WHITE;
    }
    else {
        pawns = board.piece_boards[BLACK_PAWNS_INDEX];
        color = BLACK;
    }
    bitboard pawn_moves;
    bitboard pawn_bit;
    while(pawns) {
        pc_loc = first_set_bit(pawns);
        pawn_bit = luts->pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(pawn_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[pc_loc];
        pawn_moves = generate_pawn_move_bitboard((square)pc_loc, board, luts) & check_mask & pin_mask;
        while(pawn_moves) {
            tar_loc = first_set_bit(pawn_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | PAWN;
            move.tar_piece = board.sq_board[tar_loc];
            size_t tar_rank = tar_loc / 8;
            if(tar_rank == RANK_8 || tar_rank == RANK_1) {
                piece color = (tar_rank == RANK_8) ? WHITE : BLACK;
                move.promotion_piece = color | KNIGHT;
                curr_moves.push_back(move);
                move.promotion_piece = color | BISHOP;
                curr_moves.push_back(move);
                move.promotion_piece = color | ROOK;
                curr_moves.push_back(move);
                move.promotion_piece = color | QUEEN;
                curr_moves.push_back(move);
            }
            else{
                move.promotion_piece = EMPTY;
                curr_moves.push_back(move);
            }
            pawn_moves = rem_first_bit(pawn_moves);
        }
        pawns = rem_first_bit(pawns);
    }
    return curr_moves;

}

vector<move_t> generate_rook_moves(board_t board, vector<move_t> curr_moves, bitboard check_mask, pin_t pin, lut_t *luts) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t move;
    move.promotion_piece = EMPTY; // default to empty for sliding pieces
    bitboard pin_mask;
    bitboard rooks;          
    piece color;
    if (board.t == W) {
        rooks = board.piece_boards[WHITE_ROOKS_INDEX];
        color = WHITE;
    }
    else {
        rooks = board.piece_boards[BLACK_ROOKS_INDEX];
        color = BLACK;
    }

    bitboard rook_moves;
    bitboard rook_bit;
    while(rooks) {
        pc_loc = first_set_bit(rooks);
        rook_bit = luts->pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(rook_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[pc_loc];
        rook_moves = generate_rook_move_bitboard((square)pc_loc, board, luts) & check_mask & pin_mask;
        while(rook_moves) {
            tar_loc = first_set_bit(rook_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | ROOK;
            move.tar_piece = board.sq_board[tar_loc];
            curr_moves.push_back(move);
            rook_moves = rem_first_bit(rook_moves);
        }
        rooks = rem_first_bit(rooks);
    }
    return curr_moves;
}

vector<move_t> generate_bishop_moves(board_t board, vector<move_t> curr_moves, bitboard check_mask, pin_t pin, lut_t *luts) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t move;
    move.promotion_piece = EMPTY; // default to empty for sliding pieces
    bitboard pin_mask;
    bitboard bishops;          
    piece color;
    if (board.t == W) {
        bishops = board.piece_boards[WHITE_BISHOPS_INDEX];
        color = WHITE;
    }
    else {
        bishops = board.piece_boards[BLACK_BISHOPS_INDEX];
        color = BLACK;
    }

    bitboard bishop_moves;
    bitboard bishop_bit;
    while(bishops) {
        pc_loc = first_set_bit(bishops);
        bishop_bit = luts->pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(bishop_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[pc_loc];
        bishop_moves = generate_bishop_move_bitboard((square)pc_loc, board, luts) & check_mask & pin_mask;
        while(bishop_moves) {
            tar_loc = first_set_bit(bishop_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | BISHOP;
            move.tar_piece = board.sq_board[tar_loc];
            curr_moves.push_back(move);
            bishop_moves = rem_first_bit(bishop_moves);
        }
        bishops = rem_first_bit(bishops);
    }
    return curr_moves;
}

vector<move_t> generate_queen_moves(board_t board, vector<move_t> curr_moves, bitboard check_mask, pin_t pin, lut_t *luts) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t move;
    move.promotion_piece = EMPTY; // default to empty for sliding pieces
    bitboard pin_mask;
    bitboard queens;          
    piece color;
    if (board.t == W) {
        queens = board.piece_boards[WHITE_QUEENS_INDEX];
        color = WHITE;
    }
    else {
        queens = board.piece_boards[BLACK_QUEENS_INDEX];
        color = BLACK;
    }

    bitboard queen_moves;
    bitboard queen_bit;
    while(queens) {
        pc_loc = first_set_bit(queens);
        queen_bit = luts->pieces[pc_loc];
        pin_mask = 0xFFFFFFFFFFFFFFFF;
        if(queen_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[pc_loc];
        queen_moves = generate_queen_move_bitboard((square)pc_loc, board, luts) & check_mask & pin_mask;
        while(queen_moves) {
            tar_loc = first_set_bit(queen_moves);
            move.start = (square)pc_loc;
            move.target = (square)tar_loc;
            move.mv_piece = color | QUEEN;
            move.tar_piece = board.sq_board[tar_loc];
            curr_moves.push_back(move);
            queen_moves = rem_first_bit(queen_moves);
        }
        queens = rem_first_bit(queens);
    }
    return curr_moves;
}

bitboard attackers_from_square(board_t board, square sq, lut_t *luts) {
    bitboard attackers = 0;
    bitboard opponent_knights;
    bitboard opponent_kings;
    bitboard opponent_pawns;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    if(board.t == W) {
        opponent_knights = board.piece_boards[BLACK_KNIGHTS_INDEX];
        opponent_kings = board.piece_boards[BLACK_KINGS_INDEX];
        opponent_pawns = board.piece_boards[BLACK_PAWNS_INDEX];
        opponent_rooks = board.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = board.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = board.piece_boards[BLACK_QUEENS_INDEX];
    }
    else {
        opponent_knights = board.piece_boards[WHITE_KNIGHTS_INDEX];
        opponent_kings = board.piece_boards[WHITE_KINGS_INDEX];
        opponent_pawns = board.piece_boards[WHITE_PAWNS_INDEX];
        opponent_rooks = board.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = board.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = board.piece_boards[WHITE_QUEENS_INDEX];
    }
    attackers |= get_knight_attacks(sq, luts) & opponent_knights;
    attackers |= get_king_attacks(sq, luts) & opponent_kings;
    attackers |= get_pawn_attacks(sq, !board.t, luts) & opponent_pawns;
    attackers |= get_rook_attacks(sq, board.all_pieces, luts) & opponent_rooks;
    attackers |= get_bishop_attacks(sq, board.all_pieces, luts) & opponent_bishops;
    attackers |= get_queen_attacks(sq, board.all_pieces, luts) & opponent_queens;
    return attackers;
}

bitboard checking_pieces(board_t board, lut_t *luts) {
    square friendly_king = (board.t == W) ? board.white_king_loc : board.black_king_loc;
    return attackers_from_square(board, friendly_king, luts);
}

int in_check(bitboard attackers) {
    if(attackers == 0) return NO_CHECK;
    else if(rem_first_bit(attackers) == 0) return SINGLE_CHECK;
    else return DOUBLE_CHECK;
}

bitboard opponent_slider_rays_to_square(board_t board, square sq, lut_t *luts) {
    // put a rook at the kings square... get the rook attacks
    // and that with the file and rank the rook is on
    // for queens, treat it like a rook if its on the same file or rank as sq
    // treat it like a bishop if its on the same diag or antidiag as sq
    bitboard res = 0;
    square attacker_loc;
    bitboard attackers;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    if(board.t == W) {
        opponent_rooks = board.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = board.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = board.piece_boards[BLACK_QUEENS_INDEX];
    }
    else {
        opponent_rooks = board.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = board.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = board.piece_boards[WHITE_QUEENS_INDEX];
    }
    bitboard rook_attacks_from_sq = get_rook_attacks(sq, board.all_pieces, luts);
    attackers = rook_attacks_from_sq & (opponent_rooks | opponent_queens);
    while(attackers) {
        attacker_loc = (square)first_set_bit(attackers);
        res |= get_rook_attacks(attacker_loc, board.all_pieces, luts) & rook_attacks_from_sq;
        attackers = rem_first_bit(attackers);    
    }
    bitboard bishop_attacks_from_sq = get_bishop_attacks(sq, board.all_pieces, luts);
    attackers = bishop_attacks_from_sq & (opponent_bishops | opponent_queens);
    while(attackers) {
        attacker_loc = (square)first_set_bit(attackers);
        res |= get_bishop_attacks(attacker_loc, board.all_pieces, luts) & bishop_attacks_from_sq;
        attackers = rem_first_bit(attackers);
    }
    return res;
}

pin_t get_pinned_pieces(board_t board, square friendly_king_loc, lut_t *luts) {
    pin_t pin;
    pin.pinned_pieces = 0;
    bitboard curr_pin;
    square pinned_piece_loc;
    bitboard opponent_rooks;
    bitboard opponent_bishops;
    bitboard opponent_queens;
    bitboard friendly_pieces;
    if(board.t == W) {
        opponent_rooks = board.piece_boards[BLACK_ROOKS_INDEX];
        opponent_bishops = board.piece_boards[BLACK_BISHOPS_INDEX];
        opponent_queens = board.piece_boards[BLACK_QUEENS_INDEX];
        friendly_pieces = board.white_pieces;
    }
    else {
        opponent_rooks = board.piece_boards[WHITE_ROOKS_INDEX];
        opponent_bishops = board.piece_boards[WHITE_BISHOPS_INDEX];
        opponent_queens = board.piece_boards[WHITE_QUEENS_INDEX];
        friendly_pieces = board.black_pieces;
    }
    bitboard king_rook_attacks = get_rook_attacks(friendly_king_loc, board.all_pieces, luts);
    bitboard king_bishop_attacks = get_bishop_attacks(friendly_king_loc, board.all_pieces, luts);
    bitboard rook_attacks;
    bitboard bishop_attacks;
    bitboard queen_attacks;
    square pc_loc;
    while(opponent_rooks) {
        pc_loc = (square)first_set_bit(opponent_rooks);
        rook_attacks = get_rook_attacks(pc_loc, board.all_pieces, luts);
        curr_pin = rook_attacks & king_rook_attacks & friendly_pieces;
        if(curr_pin){
            pin.pinned_pieces |= curr_pin;
            pinned_piece_loc = (square)first_set_bit(curr_pin);
            pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc, luts);
        }
        opponent_rooks = rem_first_bit(opponent_rooks);
    }
    while(opponent_bishops) {
        pc_loc = (square)first_set_bit(opponent_bishops);
        bishop_attacks = get_bishop_attacks(pc_loc, board.all_pieces, luts);
        curr_pin = bishop_attacks & king_bishop_attacks & friendly_pieces;
        if(curr_pin){
            pin.pinned_pieces |= curr_pin;
            pinned_piece_loc = (square)first_set_bit(curr_pin);
            pin.ray_at_sq[pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc, luts);
        }
        opponent_bishops = rem_first_bit(opponent_bishops);
    }
    while(opponent_queens) {
        pc_loc = (square)first_set_bit(opponent_queens);
        if((pc_loc % 8 == friendly_king_loc % 8) || (pc_loc / 8 == friendly_king_loc / 8)) {
            queen_attacks = get_rook_attacks(pc_loc, board.all_pieces, luts);
            curr_pin = queen_attacks & king_rook_attacks & friendly_pieces;
            if(curr_pin){
                pin.pinned_pieces |= curr_pin;
                pinned_piece_loc = (square)first_set_bit(curr_pin);
                pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc, luts);
            }
        }
        else {
            queen_attacks = get_bishop_attacks(pc_loc, board.all_pieces, luts);
            curr_pin = queen_attacks & king_bishop_attacks & friendly_pieces;
            if(curr_pin){
                pin.pinned_pieces |= curr_pin;
                pinned_piece_loc = (square)first_set_bit(curr_pin);
                pin.ray_at_sq[pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc, luts);
            }
        }
        opponent_queens = rem_first_bit(opponent_queens);
    }
    return pin;
}

vector<move_t> generate_moves(board_t board, lut_t *luts) {
    vector<move_t> moves;
    bitboard check_pieces = checking_pieces(board, luts);
    bitboard capture_mask = 0xFFFFFFFFFFFFFFFF;
    bitboard push_mask = 0xFFFFFFFFFFFFFFFF;
    square friendly_king_loc = (board.t == W) ? board.white_king_loc : board.black_king_loc;
    
    int check = in_check(check_pieces);
    if(check == DOUBLE_CHECK) {
        double_checks++; // DEBUGGING
        return generate_king_moves(board, moves, luts);
    }
    else if (check == SINGLE_CHECK) {
        checks++; // DEBUGGING
        capture_mask = check_pieces;
        square sq = (square)first_set_bit(check_pieces);
        if(is_sliding_piece(board.sq_board[sq])) {
            push_mask = opponent_slider_rays_to_square(board, friendly_king_loc, luts);
        }
        else {
            push_mask = 0;
        }
    }
    bitboard check_mask = push_mask | capture_mask;
    // bitboard check_mask = 0xFFFFFFFFFFFFFFFF;
    pin_t pin = get_pinned_pieces(board, friendly_king_loc, luts);
    // pin_t pin;
    // pin.pinned_pieces = 0;
    moves = generate_king_moves(board, moves, luts);
    moves = generate_knight_moves(board, moves, check_mask, pin, luts);
    moves = generate_pawn_moves(board, moves, check_mask, pin, luts);
    moves = generate_rook_moves(board, moves, check_mask, pin, luts);
    moves = generate_bishop_moves(board, moves, check_mask, pin, luts);
    moves = generate_queen_moves(board, moves, check_mask, pin, luts);
    return moves;
}

/*
 * Goes from a move struct to the correct notation, given a move, a list of 
 * legal moves in the position, and the state of the board.
 */
string notation_from_move(move_t move, vector<move_t> all_moves, board_t board) {
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
    char piece_name = pieces[bitboard_index_from_piece(move.mv_piece) / 2];
    bool capture = (move.tar_piece == EMPTY) ? false : true;
    if(move.target == board.en_passant) capture = true;
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
                pieces[bitboard_index_from_piece(move.promotion_piece) / 2]);
    }
    return str_move;
}

// not complete... probably the more useful function
// move_t move_from_notation(string str_move, vector<move_t> all_moves, board_t board) {
//     return;
// }

void print_moves(vector<move_t> move_vector) {
    for(size_t i = 0; i < move_vector.size(); i++) {
        cout << move_vector[i].start << "->" << move_vector[i].target << endl;
    }
    return;
}

size_t num_nodes(stack<board_t> board, size_t depth, string curr_not, lut_t *luts) {
    vector<move_t> moves;
    board_t curr_board = board.top();
    board_t next_board;
    if(depth == 0) {
        // print_squarewise(curr_board.sq_board); // prints the final node
        // cout << endl << "Done with this branch!" << endl;
        cout << curr_not << endl;
        return 1;
    }

    size_t total_moves = 0;
    moves = generate_moves(curr_board, luts);
    for(move_t move : moves) {
        next_board = make_move(curr_board, move, luts);
        board.push(next_board);
        string next = curr_not + notation_from_move(move, moves, curr_board) + (string)"->";
        // print_squarewise(next_board.sq_board);
        total_moves += num_nodes(board, depth - 1, next, luts); 
        board.pop();
    }
    return total_moves;
}

// void speed_test(string pos, lut_t *luts) {
//     board_t board = decode_fen(pos, luts);
//     board.t = W;
//     stack<board_t> board_stack;
//     board_stack.push(board);
//     size_t max_depth;
//     clock_t tStart;
//     clock_t tStop;
//     double time_elapsed;
//     size_t nodes;
//     cout << endl << "Max depth: ";
//     cin >> max_depth;
//     cout << endl << "Testing for speed and correctness with a max depth of " 
//          << max_depth << "..." << endl << endl;
//     for (size_t depth = 0; depth <= max_depth; depth++) {
//         cout << "Depth = " << depth << endl;
//         tStart = clock();
//         nodes = num_nodes(board_stack, depth, luts);
//         tStop = clock();
//         time_elapsed = (double)(tStop - tStart)/CLOCKS_PER_SEC;
//         cout << "Nodes reached: " << nodes << endl;
//         cout << "Captures: " << capture_moves << endl;
//         cout << "En passants: " << en_passant_moves << endl;
//         cout << "Castles: " << castle_moves << endl;
//         cout << "Promotions: " << promotion_moves << endl;
//         cout << "Checks: " << checks << endl;
//         cout << "Double Checks: " << double_checks << endl;

//         cout << "Time elapsed: " << time_elapsed << endl;
//         cout << "Nodes per second: " << ((double)nodes / time_elapsed) << endl << endl;
//     }
//     return;
// }

int main() {
    lut_t *luts = init_LUT();
    string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"; // 20 black and white
    string rook_pos = "8/8/8/2R2R2/8/2r2R2/8/8"; // 25 white, 9 black
    string rook_knight_pos = "7N/8/8/2R2R2/3N4/2r2R2/8/1N6"; // 36 white, 9 black
    string clogged_rook_pos = "RRRRRRRR/RRRRRRRR/RRRRRRRR/RRRRRRRR/RRRrRRRR/RRRRRRRR/RRRRRRRR/RRRRRRRR"; // 4 moves white and black
    string non_diag_test = "7k/pn1prp2/P3Pn2/3N4/8/5R2/4P3/3K2R1"; // 39 white, 19 black
    string bishop_test = "7b/8/8/2r5/1B6/8/6B1/B7"; // 22 white, 21 black
    string queen_test = "7Q/8/2q5/3QQ3/8/8/8/8"; // 59 white, 21 black
    string weird_test = "8/8/6Q1/8/2NbR3/2NB4/1PPP4/1B4R1";
    string en_passant_test = "k7/5p2/8/8/4P3/8/8/K7";
    string promotion_test = "8/8/5P2/8/8/6p1/8/8";
    string white_capture_promo_test = "5n2/4P3/8/8/8/8/8/8";
    string black_capture_promo_test = "8/8/8/8/8/8/4p3/5N2";
    string many_pawns_test = "8/4p1p1/2p5/pP1P2P1/4P2P/8/8/8";
    string castling_test = "r3k2r/8/8/8/8/8/8/R3K2R";
    string king_move_test = "4k3/8/2r5/8/8/2K5/8/8"; // 6 legal white
    string double_check_test = "k2r4/8/8/8/4Q3/5N2/2b5/3K4";
    string temp_test = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R";
    board_t board = decode_fen(starting_pos, luts);
    stack<board_t> board_stack;
    board_stack.push(board);

    // print_bitboard(get_pinned_pieces(board, E4, luts).ray_at_sq[C4]);

    // print_bitboard(get_ray_from_queen_to_king(E1, A5, luts));
    // print_bitboard(get_pinned_pieces(board, B1, luts));

    // print_bitboard(checking_pieces(board, luts));

    // speed_test(starting_pos, luts);

    size_t depth;
    while(true) {
        cout << endl << "Enter depth: ";
        cin >> depth;
        cout << endl << num_nodes(board_stack, depth, "", luts) << endl;
    }

    int test;
    cin >> test;
    
    free(luts);
    return 0;
}

/* still need to deal with en passant check evasions 
   now that I have the pinned pieces, pass that on to move generation...
   if its a knight, just temporarily remove it from the board when calculating knight moves
   if its a bishop or queen or rook or pawn, remove it from the board temporarily
   and make a new function to get the ray between the king and another piece

   pass in the pinned bishops to generate bishop_moves etc...

   get_ray_from_bishop_to_king(square bishop, square king, lut_t *luts)
   get_ray_from_rook_to_king(square rook, square king, lut_t *luts)

   ignoring all possible obstructing pieces (don't take in the board as an argument)
*/

/*
    Notes:
        - Maybe have a move struct and then store an array of moves in a 
          given position. Then you only need to malloc one array.
        - Have a piece-wise representation in order to figure out what piece is
          captured.
        - Have the make move function not even check for legality, that should
          be the job of the move generation.
        - Store the moves from generation in a vector (need to import library?)
        - DON'T create new copies of the board, instead make the move struct
          hold enough info to be able to undo the move
        - ^^^ need to be sure to undo the move in recursive calls
        - make LUT on the stack cuz it doesn't change
        - ^^^ maybe not tho cuz its large
        - for dealing with captured pieces, maybe make the board struct hold
          the most recent captured pieces (NONE if none captured) in order to 
          undo the move. Other stuff would also need to be stored for this to 
          work.
        - make FEN -> board function
        - I should have a better way of representing pieces in the piece array
        - 01000 = white
        - 10000 = black
        - 00001 = pawn
        - change the way that I store the twelve bitboards... maybe an array
          that is indexed by the enums?
        - I think I will need to create a new board for every new move...
        - However, I can simply an array of a fuckton of boards = depth of the
          search. Note about the non-fixed depth due to completing capture
          chains.
        - Read the wiki page... need to do bitscanning
        - in terms of searching, the array positions will be sequentially 
          ordered by board history. So board[0] would be the starting position.
        - tomorrow start with knight moves LUT generation
        - consider using stacks for the board history
        - unmake move would just be popping from the stack
        - do all pseudo-legal moves now, then cycle back once I have all the
          squares that are being attacked to deal with check
        - I think I want the board struct to hold the turn and castling rights
          info. So I have to move the turn into that.
        - Use memcpy when making a move
        - rotate 45 degrees, look at rank_attacks for given pattern, mask
          either the diagonal or antidiagonal
        - after queens are implemented, do a lot of code cleaning and commenting
          and precalculate knights, bishops, and pawns (maybe delay this)
        - next up is en passant, pawn promotion, and castling (assume pseudolegal for now)
        - still need to update make_move function and pawn move generation function
        - do knight generation in an LUT
        - my num moves function is correct. It should be called num nodes,
          because it really tells me how many nodes there are at depth = 0
        - pawn moves might currently be slow, maybe have two lookup tables,
          one for pawn captures and one for pawn pushes. Then just do en passant
          for the given pawn during the game.
        1. create the knight_attacks LUT done
        2. create the king_attacks LUT done 
           might also be worth doing a castling lut here then in generate_king_moves,
           give the rank and it can check to see if castling is legal
        3. create the pawn_attacks_white and pawn_attacks_black LUTs
           create the white_pawn_pushes
           create the white_en_passant and black_en_passant LUTs
           ^^^ don't need these, just use the pawn attacks and see if you are
           attacking an en passant square
        use these LUTs to make the attack map for white and black

        next up:
        castling...
        1. make the threat map using the LUTs given a position, maybe do this
           where you pass in all pieces except the king.

        Ideas for efficiency:
            - make a function to actually test efficiency
            - throw out updated boards and instead incrementally update the boards
              inside of make_move.
            - update the attack maps incrementally
            - castling move generation is ugly and probably bad
            - change sliding piece move generation
            - switch to magic bitboards
            - look over the bookmarked tabs to explore options
            - change representation of moves to a 32 bit integer
            - a lot of speed has to do with a good eval function

        Make it so rooks and bishops do not attack the square they are on
        store the black and white king positions in the board

        RECALTULATING THE ATTACK MAPS IS REALLLLLYYYYY TIME CONSUMING
        HIGHLY CONSIDER MAKING A REMAP FUNCTION THAT TAKES IN A MOVE AND UPDATES
        IT
            for this you can probably just recalculate the pieces on the file,
            rank, diagonal, and antidiagonal you are on

            im also not sure if I need to update black AND white's attack maps
            after every move... maybe only update the side that just moved?

        I should definitely differentiate between king_danger_squares and 
        attacked squares with two different bitboards stored in the board, only
        updating each when I need them. When do I need the attacked squares bitboard?
        
        should separate the generate_leaping_moves into each of the pieces
        so I can call them specifically when in check / double check.
        do this first ^^^^^
        actually im dumb I do have these seperate functions, but the actual
        move vector versions don't exist

        maybe a faster way to do checks is to put each piece type on the kings
        square (also keep track of kings square in board) and then see if any of
        the moves actually land on the piece we put there.
        do this ^^ and return the square being checked from
        also maybe have is_double_check function to do first

        make generate_blocking_moves

        I would still need to update the king's danger squares pretty often...

        definitely need to incrementally update the attack maps

        game plan... make it work, then make it faster

        run a function that returns a vector of pieces attacking a square
        maybe make a struct called attacker that stores the piece and the 
        square it is attacking from

        ^^ actually just return a bitboard of the attackers on a given square
*/