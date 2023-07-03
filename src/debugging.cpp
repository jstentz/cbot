#include "../include/debugging.h"
#include "../include/bitboard.h"
#include "../include/board.h"
#include "../include/pieces.h"
#include "../include/moves.h"

#include <bitset>
#include <stddef.h>
#include <iostream>
#include <stdio.h>

void print_moves(std::vector<move_t> moves) {
  for(move_t move : moves) {
    std::cout << notation_from_move(move) << ": ";
    std::cout << SCORE(move) << std::endl;
  }
}

void print_bitboard(bitboard b) {
  std::bitset<64> bs(b);
  for (size_t i = 0; i < 8; i++) {
    for (size_t j = 0; j < 8; j++) {
      std::cout << bs[(7-i)*8 + j];
    }
    std::cout << std::endl;
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
    std::cout << std::endl;
  }
  std::cout << std::endl;
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

  std::cout << "All Pieces" << std::endl;
  print_bitboard(all_pieces);

  std::cout << std::endl << "White Pieces" << std::endl;
  print_bitboard(white_pieces);

  std::cout << std::endl <<  "Black Pieces" << std::endl;
  print_bitboard(black_pieces);

  std::cout << std::endl <<  "White Pawns" << std::endl;
  print_bitboard(white_pawns);

  std::cout << std::endl <<  "Black Pawns" << std::endl;
  print_bitboard(black_pawns);

  std::cout << std::endl <<  "White Knights" << std::endl;
  print_bitboard(white_knights);

  std::cout << std::endl <<  "Black Knights" << std::endl;
  print_bitboard(black_knights);

  std::cout << std::endl <<  "White Bishops" << std::endl;
  print_bitboard(white_bishops);

  std::cout << std::endl <<  "Black Bishops" << std::endl;
  print_bitboard(black_bishops);

  std::cout << std::endl <<  "White Rooks" << std::endl;
  print_bitboard(white_rooks);

  std::cout << std::endl <<  "Black Rooks" << std::endl;
  print_bitboard(black_rooks);

  std::cout << std::endl <<  "White Queens" << std::endl;
  print_bitboard(white_queens);

  std::cout << std::endl <<  "Black Queens" << std::endl;
  print_bitboard(black_queens);

  std::cout << std::endl <<  "White Kings" << std::endl;
  print_bitboard(white_kings);

  std::cout << std::endl <<  "Black Kings" << std::endl;
  print_bitboard(black_kings);

  std::cout << std::endl <<  "Square-wise" << std::endl;
  print_squarewise(board->sq_board);
  return;
}