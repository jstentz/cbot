#include <cstdio>
#include <iostream>
#include <cmath>

using namespace std;

typedef long long unsigned int bitboard;
typedef short unsigned int uint16_t;

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
    bitboard white_pawns;
    bitboard white_knights;
    bitboard white_bishops;
    bitboard white_rooks;
    bitboard white_queens;
    bitboard white_kings;

    bitboard black_pawns;
    bitboard black_knights;
    bitboard black_bishops;
    bitboard black_rooks;
    bitboard black_queens;
    bitboard black_kings;

    bitboard white_pieces;
    bitboard black_pieces;
    bitboard all_pieces;
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
    return luts;
}

board_t *zero_board() {
    board_t *board = (board_t *) malloc(sizeof(board_t));
    board->white_pawns = 0;
    board->white_knights = 0;
    board->white_bishops = 0;
    board->white_rooks = 0;
    board->white_queens = 0;
    board->white_kings = 0;

    board->black_pawns = 0;
    board->black_knights = 0;
    board->black_bishops = 0;
    board->black_rooks = 0;
    board->black_queens = 0;
    board->black_kings = 0;

    board->white_pieces = 0;
    board->black_pieces = 0;
    board->all_pieces = 0;
    return board;
}

void update_boards(board_t *board) {
    board->white_pieces = (board->white_pawns   | board->white_knights | 
                           board->white_bishops | board->white_rooks   |
                           board->white_queens  | board->white_kings);

    board->black_pieces = (board->black_pawns   | board->black_knights | 
                           board->black_bishops | board->black_rooks   |
                           board->black_queens  | board->black_kings);

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

int main(){
    board_t *board = zero_board();
    lut_t *luts = init_LUT();
    cout << hex << uppercase << luts->pieces[63] << endl;

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

*/