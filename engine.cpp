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
#define WHITE_BISHOP_INDEX (bitboard_index_from_piece(WHITE | BISHOP))
#define BLACK_BISHOP_INDEX (bitboard_index_from_piece(BLACK | BISHOP))
#define WHITE_ROOK_INDEX (bitboard_index_from_piece(WHITE | ROOK))
#define BLACK_ROOK_INDEX (bitboard_index_from_piece(BLACK | ROOK))
#define WHITE_QUEEN_INDEX (bitboard_index_from_piece(WHITE | QUEEN))
#define BLACK_QUEEN_INDEX (bitboard_index_from_piece(BLACK | QUEEN))
#define WHITE_KING_INDEX (bitboard_index_from_piece(WHITE | KING))
#define BLACK_KING_INDEX (bitboard_index_from_piece(BLACK | KING))



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
} lut_t;

typedef struct move_struct {
    square start;
    square target;
} move_t;

lut_t *init_LUT () {
    lut_t *luts = (lut_t *) malloc(sizeof(lut_t));

    bitboard rank = 0x00000000000000FF;
    bitboard file = 0x8080808080808080;

    for(size_t i = 0; i < 8; i++) {
        luts->mask_rank[i] = rank;
        luts->clear_rank[i] = ~rank;
        luts->mask_file[i] = file;
        luts->clear_file[i] = ~file;

        rank = rank << 8;
        file = file >> 1;
    }

    bitboard piece = 0x0000000000000001;
    for(size_t i = 0; i < 64; i++) {
        luts->pieces[i] = piece;
        piece = piece << 1;
    }

    // creating knight move LUT


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

bitboard generate_knight_moves(bitboard knight, bitboard own_pieces, lut_t *luts) {
    bitboard spot_1_clip = luts->clear_file[FILE_A] & luts->clear_file[FILE_B];
    bitboard spot_2_clip = luts->clear_file[FILE_A];
    bitboard spot_3_clip = luts->clear_file[FILE_H];
    bitboard spot_4_clip = luts->clear_file[FILE_H] & luts->clear_file[FILE_G];

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

// generates the moves in the position given the turn and returns number of moves in a position
size_t generate_moves(board_t *board, move_t *move_list, lut_t *luts, turn t) {
    size_t curr_move = 0;
    
    bitboard own_pieces;
    if (t == W) own_pieces = board->white_pieces;
    else        own_pieces = board->black_pieces;

    // generate knight moves
    bitboard knights;
    if (t == W) knights = board->piece_boards[WHITE_KNIGHTS_INDEX];
    else        knights = board->piece_boards[BLACK_KNIGHTS_INDEX];

    bitboard knight_attacks;
    bitboard one_knight;
    uint16_t pc_loc;
    uint16_t tar_loc;
    while(knights) {
        pc_loc = first_set_bit(knights);
        one_knight = luts->pieces[pc_loc];
        knight_attacks = generate_knight_moves(one_knight, own_pieces, luts);
        
        while(knight_attacks) {
            tar_loc = first_set_bit(knight_attacks);
            move_list[curr_move].start = (square)pc_loc;
            move_list[curr_move].target = (square)tar_loc;
            print_bitboard(knight_attacks);
            knight_attacks = rem_first_bit(knight_attacks);
            curr_move++;
        }
        knights = rem_first_bit(knights);
    }

    return curr_move;
}

void print_moves(move_t *move_list, size_t num_moves) {
    for(size_t i = 0; i < num_moves; i++) {
        cout << move_list[i].start << "->" << move_list[i].target << endl;
    }
    return;
}

int main(){
    lut_t *luts = init_LUT();
    board_t *board = decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", luts);
    move_t *move_list = (move_t *) malloc(sizeof(move_t) * MAX_MOVES);
    print_board(board);

    size_t num_moves = generate_moves(board, move_list, luts, W);
    cout << endl << endl << num_moves << endl;
    print_moves(move_list, num_moves);

    cout << endl; 
    print_bitboard(rem_first_bit(0x000000000000FFF0));

    // print_bitboard(generate_knight_moves(luts->pieces[6], board->white_pieces, luts));

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
*/