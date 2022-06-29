#include "debugging.h"
#include "bitboard.h"
#include "board.h"
#include "pieces.h"
#include "moves.h"

#include <bitset>
#include <stddef.h>
#include <iostream>
#include <stdio.h>

using namespace std;

void print_moves(vector<move_t> moves, board_t *board) {
    for(move_t move : moves) {
        cout << notation_from_move(move) << ": ";
        cout << SCORE(move) << endl;
    }
}

void print_bitboard(bitboard b) {
    bitset<64> bs(b);
    for (size_t i = 0; i < 8; i++) {
        for (size_t j = 0; j < 8; j++) {
            std::cout << bs[(7-i)*8 + j];
        }
        std::cout << endl;
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
            std::cout << c;
        }
        std::cout << endl;
    }
    std::cout << endl;
}

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

    std::cout << "All Pieces" << endl;
    print_bitboard(all_pieces);

    std::cout << endl << "White Pieces" << endl;
    print_bitboard(white_pieces);

    std::cout << endl <<  "Black Pieces" << endl;
    print_bitboard(black_pieces);

    std::cout << endl <<  "White Pawns" << endl;
    print_bitboard(white_pawns);

    std::cout << endl <<  "Black Pawns" << endl;
    print_bitboard(black_pawns);

    std::cout << endl <<  "White Knights" << endl;
    print_bitboard(white_knights);

    std::cout << endl <<  "Black Knights" << endl;
    print_bitboard(black_knights);

    std::cout << endl <<  "White Bishops" << endl;
    print_bitboard(white_bishops);

    std::cout << endl <<  "Black Bishops" << endl;
    print_bitboard(black_bishops);

    std::cout << endl <<  "White Rooks" << endl;
    print_bitboard(white_rooks);

    std::cout << endl <<  "Black Rooks" << endl;
    print_bitboard(black_rooks);

    std::cout << endl <<  "White Queens" << endl;
    print_bitboard(white_queens);

    std::cout << endl <<  "Black Queens" << endl;
    print_bitboard(black_queens);

    std::cout << endl <<  "White Kings" << endl;
    print_bitboard(white_kings);

    std::cout << endl <<  "Black Kings" << endl;
    print_bitboard(black_kings);

    std::cout << endl <<  "Square-wise" << endl;
    print_squarewise(board->sq_board);
    return;
}