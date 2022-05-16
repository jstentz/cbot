#include <cstdio>
#include <iostream>
#include <cmath>
#include <string>
#include <bitset>
#include <vector>
#include <stack>

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

#define WHITE_PAWNS_INDEX (bitboard_index_from_piece(WHITE | PAWN))
#define BLACK_PAWNS_INDEX (bitboard_index_from_piece(BLACK | PAWN))
#define WHITE_KNIGHTS_INDEX (bitboard_index_from_piece(WHITE | KNIGHT))
#define BLACK_KNIGHTS_INDEX (bitboard_index_from_piece(BLACK | KNIGHT))
#define WHITE_BISHOPS_INDEX (bitboard_index_from_piece(WHITE | BISHOP))
#define BLACK_BISHOPS_INDEX (bitboard_index_from_piece(BLACK | BISHOP))
#define WHITE_ROOKS_INDEX (bitboard_index_from_piece(WHITE | ROOK))
#define BLACK_ROOKS_INDEX (bitboard_index_from_piece(BLACK | ROOK))
#define WHITE_QUEENS_INDEX (bitboard_index_from_piece(WHITE | QUEEN))
#define BLACK_QUEENS_INDEX (bitboard_index_from_piece(BLACK | QUEEN))
#define WHITE_KINGS_INDEX (bitboard_index_from_piece(WHITE | KING))
#define BLACK_KINGS_INDEX (bitboard_index_from_piece(BLACK | KING))



#define MAX_MOVES 218

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
} board_t;

typedef struct LUTs {
    bitboard clear_rank[8];
    bitboard mask_rank[8];
    bitboard clear_file[8];
    bitboard mask_file[8];
    bitboard mask_diagonal[15]; // only 15 diagonals, 8th is for alignment
    bitboard mask_antidiagonal[15];
    bitboard pieces[64];

    bitboard king_moves[64]; // these are currently unused
    bitboard pawn_moves[64];
    bitboard knight_moves[64];

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
} move_t;
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
            luts->rank_attacks[sq][pattern] = unplaced_mask << (sq - (file - LSB_to_first_1));
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
            for(size_t pattern = 0; pattern < 256; pattern++) {
                /* still need to mask the correct diagonal */
                luts->diagonal_attacks[i * 8 + j][pattern] = 
                    undo_pseudo_rotate_45_clockwise(luts->rank_attacks[rotated_sq][pattern])
                    & luts->mask_diagonal[diag];
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
            for(size_t pattern = 0; pattern < 256; pattern++) {
                /* still need to mask the correct diagonal */
                luts->antidiagonal_attacks[i * 8 + j][pattern] = 
                    undo_pseudo_rotate_45_anticlockwise(luts->rank_attacks[rotated_sq][pattern])
                    & luts->mask_antidiagonal[diag];
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
    return board;
}

board_t update_boards(board_t board) {
    board.black_pieces = (board.piece_boards[1]  | board.piece_boards[3] | 
                           board.piece_boards[5]  | board.piece_boards[7] |
                           board.piece_boards[9]  | board.piece_boards[11]);

    board.white_pieces = (board.piece_boards[0]  | board.piece_boards[2] | 
                           board.piece_boards[4]  | board.piece_boards[6] |
                           board.piece_boards[8]  | board.piece_boards[10]);

    board.all_pieces = board.white_pieces | board.black_pieces;
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

bitboard place_piece(bitboard board, square sq, lut_t *luts) {
    return board | luts->pieces[sq];
}

bitboard rem_piece(bitboard board, square sq, lut_t *luts) {
    return board & ~(luts->pieces[sq]);
}

// need to make this push onto the stack and also should change the turn
board_t make_move(board_t board, move_t move, lut_t *luts) {
    square start = move.start;
    square target = move.target;

    piece mv_piece = board.sq_board[start];
    piece tar_piece = board.sq_board[target];

    bitboard *mv_board = &board.piece_boards[bitboard_index_from_piece(mv_piece)];
    bitboard *landing_board = &board.piece_boards[bitboard_index_from_piece(mv_piece)];
    
    *mv_board = rem_piece(*mv_board, start, luts);
    *landing_board = place_piece(*landing_board, target, luts);

    if(tar_piece != EMPTY) {
        bitboard *captured_board = &board.piece_boards[bitboard_index_from_piece(tar_piece)];
        *captured_board = rem_piece(*captured_board, target, luts);
    }

    board.sq_board[start] = EMPTY;
    board.sq_board[target] = mv_piece;

    /* check for en passant move to remove the pawn being captured en passant */
    if(board.t == W && mv_piece == WHITE | PAWN && target == board.en_passant) {
        bitboard *black_pawns = &board.piece_boards[BLACK_PAWNS_INDEX];
        square pawn_square = (square)((int)board.en_passant - 8);
        *black_pawns = rem_piece(*black_pawns, pawn_square, luts);
        board.sq_board[pawn_square] = EMPTY;
    }
    else if(board.t == B && mv_piece == BLACK | PAWN && target == board.en_passant) {
        bitboard *white_pawns = &board.piece_boards[WHITE_PAWNS_INDEX];
        square pawn_square = (square)((int)board.en_passant + 8);
        *white_pawns = rem_piece(*white_pawns, pawn_square, luts);
        board.sq_board[pawn_square] = EMPTY;
    }


    /* Update en passant squares */
    if(mv_piece = WHITE | PAWN && target - start == 16) {
        board.en_passant = (square)(start + 8);
    }
    else if(mv_piece = BLACK | PAWN && start - target == 16) {
        board.en_passant = (square)(target + 8);
    }
    else {
        board.en_passant = NONE;
    }

    board.t = !board.t;
    return update_boards(board);
}

stack<board_t> unmake_move(stack<board_t> boards) {
    boards.pop();
    return boards;
}


// will have to update this with turn info, castling, and en passant
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
            }
            loc = row * 8 + col;
            board.sq_board[loc] = pc;
            place_board = &board.piece_boards[bitboard_index_from_piece(pc)];
            *place_board = place_piece(*place_board, (square)loc, luts);
            col += 1;
        }

    }
    return update_boards(board);
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

// make print_bitboard function
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

// !!!! might be worth doing this at the beginning and then using an LUT !!!!
bitboard generate_knight_moves(bitboard knight, board_t board, lut_t *luts) {
    bitboard own_pieces;
    if(board.t == W)  own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;
    
    bitboard spot_1_clip = luts->clear_file[FILE_H] & luts->clear_file[FILE_G];
    bitboard spot_2_clip = luts->clear_file[FILE_H];
    bitboard spot_3_clip = luts->clear_file[FILE_A];
    bitboard spot_4_clip = luts->clear_file[FILE_A] & luts->clear_file[FILE_B];

    // exploiting symmetry in knight moves
    bitboard spot_5_clip = spot_4_clip;
    bitboard spot_6_clip = spot_3_clip;
    bitboard spot_7_clip = spot_2_clip;
    bitboard spot_8_clip = spot_1_clip;

    bitboard spot_1 = (knight & spot_1_clip) >> 6;
    bitboard spot_2 = (knight & spot_2_clip) >> 15;
    bitboard spot_3 = (knight & spot_3_clip) >> 17;
    bitboard spot_4 = (knight & spot_4_clip) >> 10;

    bitboard spot_5 = (knight & spot_5_clip) << 6;
    bitboard spot_6 = (knight & spot_6_clip) << 15;
    bitboard spot_7 = (knight & spot_7_clip) << 17;
    bitboard spot_8 = (knight & spot_8_clip) << 10;

    bitboard knight_moves = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                            spot_7 | spot_8;

    return knight_moves & ~own_pieces;           
}

bitboard generate_king_moves(bitboard king, board_t board, lut_t *luts) {
    // still need to add castling... will have to take in info from the board for that
    bitboard own_pieces;
    if(board.t == W) own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;
    
    bitboard spot_1_clip = luts->clear_file[FILE_A];
    bitboard spot_3_clip = luts->clear_file[FILE_H];
    bitboard spot_4_clip = luts->clear_file[FILE_H];

    bitboard spot_5_clip = luts->clear_file[FILE_H];
    bitboard spot_7_clip = luts->clear_file[FILE_A];
    bitboard spot_8_clip = luts->clear_file[FILE_A];

    bitboard spot_1 = (king & spot_1_clip) << 7;
    bitboard spot_2 = king << 8;
    bitboard spot_3 = (king & spot_3_clip) << 9;
    bitboard spot_4 = (king & spot_4_clip) << 1;

    bitboard spot_5 = (king & spot_5_clip) >> 7;
    bitboard spot_6 = king >> 8;
    bitboard spot_7 = (king & spot_7_clip) >> 9;
    bitboard spot_8 = (king & spot_8_clip) >> 1;

    bitboard king_moves = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                          spot_7 | spot_8;

    return king_moves & ~own_pieces;           
}

bitboard generate_pawn_moves(bitboard pawn, board_t board, lut_t *luts) {
    // will need to add en passant later
    bitboard enemy_pieces;
    bitboard all_pieces = board.all_pieces;
    bitboard spot_1_clip = luts->clear_file[FILE_A];
    bitboard spot_4_clip = luts->clear_file[FILE_H];

    bitboard white_mask_rank = luts->mask_rank[RANK_2];
    bitboard black_mask_rank = luts->mask_rank[RANK_7];

    bitboard spot_1;
    bitboard spot_2;
    bitboard spot_3;
    bitboard spot_4;

    bitboard captures;
    bitboard forward_moves;
    bitboard en_passant_capture;
    square en_passant_sq = board.en_passant;
    bitboard en_passant_bit = 0; // default it to zero
    if(en_passant_sq != NONE) {
        en_passant_bit =  luts->pieces[en_passant_sq]; // used to and with attack pattern
    }

    if(board.t == W) {
        enemy_pieces = board.black_pieces;
        spot_1 = (pawn & spot_1_clip) << 7;
        spot_4 = (pawn & spot_4_clip) << 9;
        captures = (spot_1 | spot_4) & enemy_pieces;
        en_passant_capture = (spot_1 | spot_4) & en_passant_bit;

        spot_2 = (pawn << 8) & ~all_pieces;
        spot_3 = ((((pawn & white_mask_rank) << 8) & ~all_pieces) << 8) & ~all_pieces;
        forward_moves = spot_2 | spot_3;
    }
    else {
        enemy_pieces = board.white_pieces;
        spot_1 = (pawn & spot_4_clip) >> 7;
        spot_4 = (pawn & spot_1_clip) >> 9;
        captures = (spot_1 | spot_4) & enemy_pieces;
        en_passant_capture = (spot_1 | spot_4) & en_passant_bit;

        spot_2 = (pawn >> 8) & ~all_pieces;
        spot_3 = ((((pawn & black_mask_rank) >> 8) & ~all_pieces) >> 8) & ~all_pieces;
        forward_moves = spot_2 | spot_3;
    }

    return captures | forward_moves | en_passant_capture;
}

bitboard generate_rook_moves(square rook, board_t board, lut_t *luts) {
    bitboard all_pieces = board.all_pieces;
    bitboard own_pieces;
    if(board.t == W)  own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;

    size_t rook_rank = (size_t)rook / 8;
    size_t rook_file = (size_t)rook % 8;
    bitboard rank_pattern = (board.all_pieces & luts->mask_rank[rook_rank]) >> (rook_rank * 8);
    bitboard file_pattern = rotate_90_anticlockwise(board.all_pieces & luts->mask_file[rook_file]) >> (rook_file * 8);
    return (luts->rank_attacks[rook][rank_pattern] | 
           luts->file_attacks[rook][file_pattern]) &
           ~own_pieces;
}

bitboard generate_bishop_moves(square bishop, board_t board, lut_t *luts) {
    bitboard all_pieces = board.all_pieces;
    bitboard own_pieces;

    if(board.t == W)  own_pieces = board.white_pieces;
    else              own_pieces = board.black_pieces;


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
    bitboard diag_pattern = pseudo_rotate_45_clockwise(board.all_pieces & luts->mask_diagonal[bishop_diag]) >> (8 * rotated_rank);
    size_t antirotated_rank;
    if(bishop_antidiag == 0)      antirotated_rank = 0;
    else if(bishop_antidiag < 8)  antirotated_rank = 8 - bishop_antidiag;
    else                          antirotated_rank = 8 - (bishop_antidiag - 7);
    bitboard antidiag_pattern = pseudo_rotate_45_anticlockwise(board.all_pieces & luts->mask_antidiagonal[bishop_antidiag]) >> (8 * antirotated_rank);
    return  ( luts->diagonal_attacks[bishop][diag_pattern]
            | luts->antidiagonal_attacks[bishop][antidiag_pattern]) 
            & ~own_pieces;
}

bitboard generate_queen_moves(square queen, board_t board, lut_t *luts) {
    return   generate_rook_moves(queen, board, luts)
           | generate_bishop_moves(queen, board, luts);
}
// generates the moves in the position given the turn and returns number of moves in a position
vector<move_t> generate_leaping_moves(board_t board, vector<move_t> move_vector, lut_t *luts) {
    
    move_t curr_move;
    // generate knight moves
    bitboard knights;
    if (board.t == W)  knights = board.piece_boards[WHITE_KNIGHTS_INDEX];
    else               knights = board.piece_boards[BLACK_KNIGHTS_INDEX];

    bitboard knight_attacks;
    bitboard one_knight;
    uint16_t pc_loc;
    uint16_t tar_loc;
    while(knights) {
        pc_loc = first_set_bit(knights);
        one_knight = luts->pieces[pc_loc];
        knight_attacks = generate_knight_moves(one_knight, board, luts);
        
        while(knight_attacks) {
            tar_loc = first_set_bit(knight_attacks);
            curr_move.start = (square)pc_loc;
            curr_move.target = (square)tar_loc;
            move_vector.push_back(curr_move);
            knight_attacks = rem_first_bit(knight_attacks);
        }
        knights = rem_first_bit(knights);
    }

    // generate king moves
    bitboard kings;
    if (board.t == W)  kings = board.piece_boards[WHITE_KINGS_INDEX];
    else               kings = board.piece_boards[BLACK_KINGS_INDEX];

    bitboard kings_attacks;
    bitboard one_kings;
    while(kings) {
        pc_loc = first_set_bit(kings);
        one_kings = luts->pieces[pc_loc];
        kings_attacks = generate_king_moves(one_kings, board, luts);
        
        while(kings_attacks) {
            tar_loc = first_set_bit(kings_attacks);
            curr_move.start = (square)pc_loc;
            curr_move.target = (square)tar_loc;
            move_vector.push_back(curr_move);
            kings_attacks = rem_first_bit(kings_attacks);
        }
        kings = rem_first_bit(kings);
    }

    // generate pawn moves
    bitboard pawns;
    if (board.t == W)  pawns = board.piece_boards[WHITE_PAWNS_INDEX];
    else               pawns = board.piece_boards[BLACK_PAWNS_INDEX];

    bitboard pawns_attacks;
    bitboard one_pawns;
    while(pawns) {
        pc_loc = first_set_bit(pawns);
        one_pawns = luts->pieces[pc_loc];
        pawns_attacks = generate_pawn_moves(one_pawns, board, luts);
        
        while(pawns_attacks) {
            tar_loc = first_set_bit(pawns_attacks);
            curr_move.start = (square)pc_loc;
            curr_move.target = (square)tar_loc;
            move_vector.push_back(curr_move);
            pawns_attacks = rem_first_bit(pawns_attacks);
        }
        pawns = rem_first_bit(pawns);
    }
    return move_vector;
}

vector<move_t> generate_sliding_moves(board_t board, vector<move_t> move_vector, lut_t *luts) {
    uint16_t pc_loc;
    uint16_t tar_loc;

    move_t curr_move;
    
    // generate rook moves
    bitboard rooks;
    if (board.t == W)  rooks = board.piece_boards[WHITE_ROOKS_INDEX];
    else               rooks = board.piece_boards[BLACK_ROOKS_INDEX];

    bitboard rooks_attacks;
    while(rooks) {
        pc_loc = first_set_bit(rooks);
        rooks_attacks = generate_rook_moves((square)pc_loc, board, luts);
        
        while(rooks_attacks) {
            tar_loc = first_set_bit(rooks_attacks);
            curr_move.start = (square)pc_loc;
            curr_move.target = (square)tar_loc;
            move_vector.push_back(curr_move);
            rooks_attacks = rem_first_bit(rooks_attacks);
        }
        rooks = rem_first_bit(rooks);
    }

    // generate bishop moves
    bitboard bishops;
    if (board.t == W)  bishops = board.piece_boards[WHITE_BISHOPS_INDEX];
    else               bishops = board.piece_boards[BLACK_BISHOPS_INDEX];

    bitboard bishops_attacks;
    while(bishops) {
        pc_loc = first_set_bit(bishops);
        bishops_attacks = generate_bishop_moves((square)pc_loc, board, luts);
        
        while(bishops_attacks) {
            tar_loc = first_set_bit(bishops_attacks);
            curr_move.start = (square)pc_loc;
            curr_move.target = (square)tar_loc;
            move_vector.push_back(curr_move);
            bishops_attacks = rem_first_bit(bishops_attacks);
        }
        bishops = rem_first_bit(bishops);
    }

    // generate bishop moves
    bitboard queens;
    if (board.t == W)  queens = board.piece_boards[WHITE_QUEENS_INDEX];
    else               queens = board.piece_boards[BLACK_QUEENS_INDEX];

    bitboard queens_attacks;
    while(queens) {
        pc_loc = first_set_bit(queens);
        queens_attacks = generate_queen_moves((square)pc_loc, board, luts);
        
        while(queens_attacks) {
            tar_loc = first_set_bit(queens_attacks);
            curr_move.start = (square)pc_loc;
            curr_move.target = (square)tar_loc;
            move_vector.push_back(curr_move);
            queens_attacks = rem_first_bit(queens_attacks);
        }
        queens = rem_first_bit(queens);
    }
    return move_vector;
}

vector<move_t> generate_moves(board_t board, lut_t *luts) {
    vector<move_t> empty_vector;
    vector<move_t> leaping_moves = generate_leaping_moves(board, empty_vector, luts);
    return generate_sliding_moves(board, leaping_moves, luts);
}

void print_moves(vector<move_t> move_vector) {
    for(size_t i = 0; i < move_vector.size(); i++) {
        cout << move_vector[i].start << "->" << move_vector[i].target << endl;
    }
    return;
}

size_t num_moves(stack<board_t> board, size_t depth, lut_t *luts) {
    vector<move_t> moves;
    board_t curr_board = board.top();
    board_t next_board;
    print_squarewise(curr_board.sq_board);
    
    if(depth == 0) {
        return 1;
    }

    size_t total_moves = 0;
    moves = generate_moves(curr_board, luts);
    for(move_t move : moves) {
        next_board = make_move(curr_board, move, luts);
        board.push(next_board);
        total_moves += num_moves(board, depth - 1, luts); 
        board.pop();
    }
    return total_moves;
}

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
    board_t board = decode_fen(en_passant_test, luts);
    stack<board_t> board_stack;
    board_stack.push(board);
    // print_board(board);

    cout << endl << num_moves(board_stack, 4, luts) << endl;
    // vector<move_t> moves = generate_moves(board, luts);
    // cout << endl << moves.size() << endl;
    // print_moves(moves);
    
    int x;
    cin >> x;
    free(luts);
    return 0;
}

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
*/