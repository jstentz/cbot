#pragma once

#include <string>

namespace constants
{

const std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::string RANKS = "12345678";
const std::string FILES = "abcdefgh";
const std::string PIECES = "PNBRQK";

const size_t NUM_PIECE_TYPES = 12; // white knight, black king, etc...

enum Square : int { A1, B1, C1, D1, E1, F1, G1, H1,
                                 A2, B2, C2, D2, E2, F2, G2, H2,
                                 A3, B3, C3, D3, E3, F3, G3, H3,
                                 A4, B4, C4, D4, E4, F4, G4, H4,
                                 A5, B5, C5, D5, E5, F5, G5, H5,
                                 A6, B6, C6, D6, E6, F6, G6, H6,
                                 A7, B7, C7, D7, E7, F7, G7, H7,
                                 A8, B8, C8, D8, E8, F8, G8, H8, NONE };

enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

const char WHITE_PAWNS_INDEX = 0;
const char BLACK_PAWNS_INDEX = 1;
const char WHITE_KNIGHTS_INDEX = 2;
const char BLACK_KNIGHTS_INDEX = 3;
const char WHITE_BISHOPS_INDEX = 4;
const char BLACK_BISHOPS_INDEX = 5;
const char WHITE_ROOKS_INDEX = 6;
const char BLACK_ROOKS_INDEX = 7;
const char WHITE_QUEENS_INDEX = 8;
const char BLACK_QUEENS_INDEX = 9;
const char WHITE_KINGS_INDEX = 10;
const char BLACK_KINGS_INDEX = 11;

const int piece_values[10] = {100, // white pawn
               -100, // black pawn
                320, // white knight
               -320, // black knight
                330, // white bishop
               -330, // black bishop
                500, // white rook
               -500, // black rook
                900, // white queen
               -900};// black queen

const int white_pawns_score[64] = 
{
  0,   0,   0,   0,   0,   0,   0,  0,
  5,  10,  10, -20, -20,  10,  10,  5,
  5,  -5, -10,   0,   0, -10,  -5,  5,
  0,   0,   0,  20,  20,   0,   0,  0,
  5,   5,  10,  25,  25,  10,   5,  5,
   10,  10,  20,  30,  30,  20,  10, 10,
   50,  50,  50,  50,  50,  50,  50, 50,
  0,   0,   0,   0,   0,   0,   0,  0
};

const int black_pawns_score[64] = 
{
  0,   0,   0,   0,   0,   0,   0,  0,
  -50, -50, -50, -50, -50, -50, -50,-50,
   -10,  -10,  -20,  -30,  -30,  -20,  -10, -10,
  -5,   -5,  -10,  -25,  -25,  -10,   -5,  -5,
  0,   0,   0,  -20,  -20,   0,   0,  0,
  -5,  5, 10,   0,   0, 10,  5,  -5,
  -5,  -10,  -10, 20, 20,  -10,  -10,  -5,
  0,   0,   0,   0,   0,   0,   0,  0
};

const int white_knights_score[64] = 
{
  -50, -40, -30, -30, -30, -30, -40, -50,
  -40, -20,   0,   5,   5,   0, -20, -40,
  -30,   5,  10,  15,  15,  10,   5, -30,
  -30,   0,  15,  20,  20,  15,   0, -30,
  -30,   5,  15,  20,  20,  15,   5, -30,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -40, -20,   0,   0,   0,   0, -20, -40,
  -50, -40, -30, -30, -30, -30, -40, -50
};

const int black_knights_score[64] = 
{
  50, 40, 30, 30, 30,  30, 40, 50,
  40, 20,   0,   0,   0,   0, 20, 40,
  30,   0,  -10,  -15,  -15,  -10,   0, 30,
  30,   -5,  -15,  -20,  -20,  -15,   -5, 30,
  30,   0,  -15,  -20,  -20,  -15,   0, 30,
  30,   -5,  -10,  -15,  -15,  -10,   -5, 30,
  40, 20,   0,   -5,   -5,   0, 20, 40,
  50, 40, 30, 30, 30, 30, 40, 50
};

const int white_bishops_score[64] = 
{
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,   5,   0,   0,   0,   0,   5, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   0,  10,  10,  10,  10,   0, -10,
  -10,   5,   5,  10,  10,   5,   5, -10,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10, -10, -10, -10, -10, -20
};

const int black_bishops_score[64] = 
{
  20, 10, 10, 10, 10, 10, 10, 20,
  10,   0,   0,   0,   0,   0,   0, 10,
  10,   0,   -5,  -10,  -10,   -5,   0, 10,
  10,   -5,   -5,  -10,  -10,   -5,   -5, 10,
  10,   0,  -10,  -10,  -10,  -10,   0, 10,
  10,  -10,  -10,  -10,  -10,  -10,  -10, 10,
  10,   -5,   0,   0,   0,   0,   -5, 10,
  20, 10, 10, 10, 10, 10, 10, 20
};

const int white_rooks_score[64] = 
{
   0,    0,   0,   5,   5,   0,   0,   0,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
  -5,    0,   0,   0,   0,   0,   0,  -5,
   5,   10,  10,  10,  10,  10,  10,   5,
   0,    0,   0,   0,   0,   0,   0,   0
};

const int black_rooks_score[64] = 
{
   0,    0,   0,   0,   0,   0,   0,   0,
   -5,   -10,  -10,  -10,  -10,  -10,  -10,   -5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
  5,    0,   0,   0,   0,   0,   0,  5,
   0,    0,   0,   -5,   -5,   0,   0,   0
};

const int white_queens_score[64] = 
{
 -20,  -10, -10,  -5,  -5, -10, -10,  -20,
 -10,    0,   5,   0,   0,   0,   0, -10,
 -10,    5,   5,   5,   5,   5,   0, -10,
   0,    0,   5,   5,   5,   5,   0,  -5,
  -5,    0,   5,   5,   5,   5,   0,  -5,
 -10,    0,   5,   5,   5,   5,   0, -10,
 -10,    0,   0,   0,   0,   0,   0, -10,
 -20,  -10, -10,  -5,  -5, -10, -10,  -20
};

const int black_queens_score[64] = 
{
 20,  10, 10,  5,  5, 10, 10,  20,
 10,    0,   0,   0,   0,   0,   0, 10,
 10,    0,   -5,   -5,   -5,   -5,   0, 10,
  5,    0,   -5,   -5,   -5,   -5,   0,  5,
   0,    0,   -5,   -5,   -5,  -5,   0,  5,
 10,    -5,   -5,   -5,   -5,   -5,   0, 10,
 10,    0,   -5,   0,   0,   0,   0, 10,
 20,  10, 10,  5,  5, 10, 10, 20
};

const int white_king_middlegame_score[64] = 
{
   20, 30, 10,  0,  0, 10, 30, 20,
   20, 20,  0,  0,  0,  0, 20, 20,
  -10,-20,-20,-20,-20,-20,-20,-10,
  -20,-30,-30,-40,-40,-30,-30,-20,
  -30,-40,-40,-50,-50,-40,-40,-30,
  -30,-40,-40,-50,-50,-40,-40,-30,
  -30,-40,-40,-50,-50,-40,-40,-30,
  -30,-40,-40,-50,-50,-40,-40,-30
};

const int black_king_middlegame_score[64] = 
{
  30,40,40,50,50,40,40,30,
  30,40,40,50,50,40,40,30,
  30,40,40,50,50,40,40,30,
  30,40,40,50,50,40,40,30,
  20,30,30,40,40,30,30,20,
  10,20,20,20,20,20,20,10,
   -20, -20,  0,  0,  0,  0, -20, -20,
   -20, -30, -10,  0,  0, -10, -30, -20
};

const int white_king_endgame_score[64] = 
{
  -50,-30,-30,-30,-30,-30,-30,-50,
  -30,-30,  0,  0,  0,  0,-30,-30,
  -30,-10, 20, 30, 30, 20,-10,-30,
  -30,-10, 30, 40, 40, 30,-10,-30,
  -30,-10, 30, 40, 40, 30,-10,-30,
  -30,-10, 20, 30, 30, 20,-10,-30,
  -30,-20,-10,  0,  0,-10,-20,-30,
  -50,-40,-30,-20,-20,-30,-40,-50
};

const int black_king_endgame_score[64] = 
{
  50,40,30,20,20,30,40,50,
  30,20,10,  0,  0,10,20,30,
  30,10, -20, -30, -30, -20,10,30,
  30,10, -30, -40, -40, -30,10,30,
  30,10, -30, -40, -40, -30,10,30,
  30,10, -20, -30, -30, -20,10,30,
  30,30,  0,  0,  0,  0,30,30,
  50,30,30,30,30,30,30,50
};

/**
 * @brief Contains an array of piece-location scores from white's perspective.
 * 
 */
const int* piece_scores[14] = 
{
  white_pawns_score,
  black_pawns_score,
  white_knights_score,
  black_knights_score,
  white_bishops_score,
  black_bishops_score,
  white_rooks_score,
  black_rooks_score,
  white_queens_score,
  black_queens_score,
  white_king_middlegame_score,
  black_king_middlegame_score,
  white_king_endgame_score,
  black_king_endgame_score
};

/// TODO: make these part of the parameterization 
// this is also the number of entries... the real size is num_entries * sizeof(Entry)
const size_t SEARCH_TT_SIZE = 131072; // 2^17 THIS MUST BE A POWER OF 2
const size_t EVAL_TT_SIZE = 131072; // 2^17 THIS MUST BE A POWER OF 2

}