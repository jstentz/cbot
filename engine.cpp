#include <cstdio>
#include <iostream>
#include <cmath>

using namespace std;

typedef long long unsigned int bitboard;
typedef short unsigned int uint16_t;

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

}

// returns the 0-indexed location of the first 1 bit from LSB to MSB
// also returns 0 when the input is 0 (so case on that)
uint16_t first_set_bit(bitboard bits) {
    return log2(bits & -bits);
}

int main(){
    board_t *board = zero_board();
    cout << first_set_bit(0x0000000000000000);
    int x;
    cin >> x;
    return 0;
}