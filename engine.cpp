#include <cstdio>
#include <iostream>
#include <cmath>
#include <string>
#include <bitset>

using namespace std;

typedef long long unsigned int bitboard;
typedef short unsigned int uint16_t;
typedef short unsigned int piece;

#define WHITE (piece)0x0
#define BLACK (piece)0x1
#define PAWN (piece)0x2
#define KNIGHT (piece)0x4
#define BISHOP (piece)0x6
#define ROOK (piece)0x8
#define QUEEN (piece)0xA
#define KING (piece)0xC
#define EMPTY (piece)0x0

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
    int loc = 0;
    for (size_t i = 0; i < fen.size(); i++) {
        char c = fen[fen.size() - i - 1];
        pc = 0;
        if (isdigit(c)) {
            loc += c - '0'; // adds c onto loc
        }
        else if (c == '/') {
            continue;
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
            board->sq_board[loc] = pc;
            place_board = &board->piece_boards[bitboard_index_from_piece(pc)];
            *place_board = place_piece(*place_board, (square)loc, luts);
            loc += 1;
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

int main(){
    lut_t *luts = init_LUT();
    board_t *board = decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", luts);

    print_board(board);

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



*/