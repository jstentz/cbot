#include <cstdio>
#include <iostream>
#include <cmath>
#include <string>
#include <bitset>

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
              A8, B8, C8, D8, E8, F8, G8, H8 };

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
} board_t;

typedef struct LUTs {
    bitboard clear_rank[8];
    bitboard mask_rank[8];
    bitboard clear_file[8];
    bitboard mask_file[8];
    bitboard pieces[64];

    bitboard king_moves[64];
    bitboard pawn_moves[64];
    bitboard knight_moves[64];

    bitboard rank_attacks[64][256]; 
    bitboard file_attacks[64][256];
} lut_t;

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

bitboard rotate_90_anti_clockwise(bitboard b) {
    return flip_diag_a1_h8(flip_vertical(b));
}

bitboard rotate_90_clockwise(bitboard b) {
    return flip_vertical(flip_diag_a1_h8(b));
}

lut_t *init_LUT () {
    lut_t *luts = (lut_t *) malloc(sizeof(lut_t));

    bitboard rank = 0x00000000000000FF;
    bitboard file = 0x0101010101010101;

    for(size_t i = 0; i < 8; i++) {
        luts->mask_rank[i] = rank;
        luts->clear_rank[i] = ~rank;
        luts->mask_file[i] = file;
        luts->clear_file[i] = ~file;

        rank = rank << 8;
        file = file << 1;
    }

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

    return luts;
}

size_t bitboard_index_from_piece(piece pc) {
    return pc - 2; //this is to offset the existence of the two EMPTY (0000 and 0001)
}

board_t *zero_board() {
    board_t *board = (board_t *) malloc(sizeof(board_t));
    
    for (size_t i = 0; i < 12; i++) {
        board->piece_boards[i] = 0;
    }

    board->white_pieces = 0;
    board->black_pieces = 0;
    board->all_pieces = 0;

    for (size_t i = 0; i < 64; i++) {
        board->sq_board[i] = EMPTY;
    }

    board->t = W;
    return board;
}

void update_boards(board_t *board) {
    board->black_pieces = (board->piece_boards[1]  | board->piece_boards[3] | 
                           board->piece_boards[5]  | board->piece_boards[7] |
                           board->piece_boards[9]  | board->piece_boards[11]);

    board->white_pieces = (board->piece_boards[0]  | board->piece_boards[2] | 
                           board->piece_boards[4]  | board->piece_boards[6] |
                           board->piece_boards[8]  | board->piece_boards[10]);

    board->all_pieces = board->white_pieces | board->black_pieces;
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
void make_move(board_t *board, move_t move, lut_t *luts) {
    square start = move.start;
    square target = move.target;

    piece mv_piece = board->sq_board[start];
    piece tar_piece = board->sq_board[target];

    bitboard *mv_board = &board->piece_boards[bitboard_index_from_piece(mv_piece)];
    bitboard *landing_board = &board->piece_boards[bitboard_index_from_piece(mv_piece)];
    
    *mv_board = rem_piece(*mv_board, start, luts);
    *landing_board = place_piece(*landing_board, target, luts);

    if(tar_piece != EMPTY) {
        bitboard *captured_board = &board->piece_boards[bitboard_index_from_piece(tar_piece)];
        *captured_board = rem_piece(*captured_board, target, luts);
    }

    board->sq_board[move.start] = EMPTY;
    board->sq_board[move.target] = mv_piece;
    update_boards(board);

    board->t = !board->t;
    return;
}


// will have to update this with turn info, castling, and en passant
board_t *decode_fen(string fen, lut_t *luts) {
    board_t *board = zero_board();
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
            board->sq_board[loc] = pc;
            place_board = &board->piece_boards[bitboard_index_from_piece(pc)];
            *place_board = place_piece(*place_board, (square)loc, luts);
            col += 1;
        }

    }
    update_boards(board);
    return board;
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
// make print_bitboard function
void print_board(board_t *board) {
    bitboard all_pieces = (board->all_pieces);
    bitboard white_pieces = (board->white_pieces);
    bitboard black_pieces = (board->black_pieces);

    bitboard white_pawns = (board->piece_boards[0]);
    bitboard black_pawns = (board->piece_boards[1]);
    bitboard white_knights = (board->piece_boards[2]);
    bitboard black_knights = (board->piece_boards[3]);
    bitboard white_bishops = (board->piece_boards[4]);
    bitboard black_bishops = (board->piece_boards[5]);
    bitboard white_rooks = (board->piece_boards[6]);
    bitboard black_rooks = (board->piece_boards[7]);
    bitboard white_queens = (board->piece_boards[8]);
    bitboard black_queens = (board->piece_boards[9]);
    bitboard white_kings = (board->piece_boards[10]);
    bitboard black_kings = (board->piece_boards[11]);

    piece *sqs = board->sq_board;
    piece pc;

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
    char c;
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
    return;
}

// !!!! might be worth doing this at the beginning and then using an LUT !!!!
bitboard generate_knight_moves(bitboard knight, board_t *board, lut_t *luts) {
    bitboard own_pieces;
    if(board->t == W) own_pieces = board->white_pieces;
    else              own_pieces = board->black_pieces;
    
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

bitboard generate_king_moves(bitboard king, board_t *board, lut_t *luts) {
    // still need to add castling... will have to take in info from the board for that
    bitboard own_pieces;
    if(board->t == W) own_pieces = board->white_pieces;
    else              own_pieces = board->black_pieces;
    
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

bitboard generate_pawn_moves(bitboard pawn, board_t *board, lut_t *luts) {
    // will need to add en passant later
    // generate capture moves first
    bitboard enemy_pieces;
    bitboard all_pieces = board->all_pieces;
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

    if(board->t == W) {
        enemy_pieces = board->black_pieces;
        spot_1 = (pawn & spot_1_clip) << 7;
        spot_4 = (pawn & spot_4_clip) << 9;
        captures = (spot_1 | spot_4) & enemy_pieces;

        spot_2 = (pawn << 8) & ~all_pieces;
        spot_3 = ((((pawn & white_mask_rank) << 8) & ~all_pieces) << 8) & ~all_pieces;
        forward_moves = spot_2 | spot_3;
    }
    else {
        enemy_pieces = board->white_pieces;
        spot_1 = (pawn & spot_4_clip) >> 7;
        spot_4 = (pawn & spot_1_clip) >> 9;
        captures = (spot_1 | spot_4) & enemy_pieces;

        spot_2 = (pawn >> 8) & ~all_pieces;
        spot_3 = ((((pawn & black_mask_rank) >> 8) & ~all_pieces) >> 8) & ~all_pieces;
        forward_moves = spot_2 | spot_3;
    }

    return captures | forward_moves;
}

bitboard generate_rook_moves(square rook, board_t *board, lut_t *luts) {
    bitboard all_pieces = board->all_pieces;
    bitboard own_pieces;
    if(board->t == W) own_pieces = board->white_pieces;
    else              own_pieces = board->black_pieces;

    size_t rook_rank = (size_t)rook / 8;
    size_t rook_file = (size_t)rook % 8;
    bitboard rank_pattern = (board->all_pieces & luts->mask_rank[rook_rank]) >> (rook_rank * 8);
    bitboard file_pattern = rotate_90_anti_clockwise(board->all_pieces & luts->mask_file[rook_file]) >> (rook_file * 8);
    return (luts->rank_attacks[rook][rank_pattern] | 
           luts->file_attacks[rook][file_pattern]) &
           ~own_pieces;
}

// generates the moves in the position given the turn and returns number of moves in a position
size_t generate_leaping_moves(board_t *board, move_t *move_list, lut_t *luts) {
    // restrict move generation due to check in this function
    size_t curr_move = 0;

    // generate knight moves
    bitboard knights;
    if (board->t == W) knights = board->piece_boards[WHITE_KNIGHTS_INDEX];
    else               knights = board->piece_boards[BLACK_KNIGHTS_INDEX];

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
            move_list[curr_move].start = (square)pc_loc;
            move_list[curr_move].target = (square)tar_loc;
            knight_attacks = rem_first_bit(knight_attacks);
            curr_move++;
        }
        knights = rem_first_bit(knights);
    }

    // generate king moves
    bitboard kings;
    if (board->t == W) kings = board->piece_boards[WHITE_KINGS_INDEX];
    else               kings = board->piece_boards[BLACK_KINGS_INDEX];

    bitboard kings_attacks;
    bitboard one_kings;
    while(kings) {
        pc_loc = first_set_bit(kings);
        one_kings = luts->pieces[pc_loc];
        kings_attacks = generate_king_moves(one_kings, board, luts);
        
        while(kings_attacks) {
            tar_loc = first_set_bit(kings_attacks);
            move_list[curr_move].start = (square)pc_loc;
            move_list[curr_move].target = (square)tar_loc;
            kings_attacks = rem_first_bit(kings_attacks);
            curr_move++;
        }
        kings = rem_first_bit(kings);
    }

    // generate pawn moves
    bitboard pawns;
    if (board->t == W) pawns = board->piece_boards[WHITE_PAWNS_INDEX];
    else               pawns = board->piece_boards[BLACK_PAWNS_INDEX];

    bitboard pawns_attacks;
    bitboard one_pawns;
    while(pawns) {
        pc_loc = first_set_bit(pawns);
        one_pawns = luts->pieces[pc_loc];
        pawns_attacks = generate_pawn_moves(one_pawns, board, luts);
        
        while(pawns_attacks) {
            tar_loc = first_set_bit(pawns_attacks);
            move_list[curr_move].start = (square)pc_loc;
            move_list[curr_move].target = (square)tar_loc;
            pawns_attacks = rem_first_bit(pawns_attacks);
            curr_move++;
        }
        pawns = rem_first_bit(pawns);
    }

    // generate rook moves
    bitboard rooks;
    if (board->t == W) rooks = board->piece_boards[WHITE_ROOKS_INDEX];
    else               rooks = board->piece_boards[BLACK_ROOKS_INDEX];

    bitboard rooks_attacks;
    while(rooks) {
        pc_loc = first_set_bit(rooks);
        rooks_attacks = generate_rook_moves((square)pc_loc, board, luts);
        
        while(rooks_attacks) {
            tar_loc = first_set_bit(rooks_attacks);
            move_list[curr_move].start = (square)pc_loc;
            move_list[curr_move].target = (square)tar_loc;
            rooks_attacks = rem_first_bit(rooks_attacks);
            curr_move++;
        }
        rooks = rem_first_bit(rooks);
    }
    return curr_move;
}

void print_moves(move_t *move_list, size_t num_moves) {
    for(size_t i = 0; i < num_moves; i++) {
        cout << move_list[i].start << "->" << move_list[i].target << endl;
    }
    return;
}

int main() {
    lut_t *luts = init_LUT();
    string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"; // 20 moves
    string rook_pos = "8/8/8/2R2R2/8/2r2R2/8/8"; // 25 moves
    string rook_knight_pos = "7N/8/8/2R2R2/3N4/2r2R2/8/1N6"; // 36 moves
    string clogged_rook_pos = "RRRRRRRR/RRRRRRRR/RRRRRRRR/RRRRRRRR/RRRrRRRR/RRRRRRRR/RRRRRRRR/RRRRRRRR"; // 4 moves
    board_t *board = decode_fen(clogged_rook_pos, luts);
    // board->t = B;
    move_t *move_list = (move_t *) malloc(sizeof(move_t) * MAX_MOVES);
    print_board(board);

    size_t num_moves = generate_leaping_moves(board, move_list, luts);
    cout << endl << endl << num_moves << endl;
    print_moves(move_list, num_moves);

    // print_bitboard(generate_pawn_moves(luts->pieces[16], board, luts));
    // print_bitboard(board->piece_boards[BLACK_PAWNS_INDEX]);
    // for(size_t i = 0; i < 64; i++) {
    //     print_bitboard(luts->file_attacks[i][8]);
    //     cout << endl;
    // }
    // print_bitboard((bitboard)0x0101000000000101);
    // cout << endl;
    // print_bitboard(rotate_90_anti_clockwise((bitboard)0x0101000000000101));
    // cout << endl;
    // print_bitboard(rotate_90_clockwise((bitboard)0x0101000000000101));
    // cout << endl;
    

    int x;
    cin >> x;
    free(board);
    free(luts);
    return 0;
}

/*
    Notes:
        - Maybe have a move struct and then store an array of moves in a 
          given position. Then you only need to malloc one array.
        - Draw things out on iPad
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
*/