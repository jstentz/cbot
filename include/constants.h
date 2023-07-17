#pragma once

#include <string>

namespace constants
{

inline std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
inline std::string RANKS = "12345678";
inline std::string FILES = "abcdefgh";
inline std::string PIECES = "PNBRQK";

inline size_t NUM_PIECE_TYPES = 12; // white knight, black king, etc...

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

inline char WHITE_PAWNS_INDEX = 0;
inline char BLACK_PAWNS_INDEX = 1;
inline char WHITE_KNIGHTS_INDEX = 2;
inline char BLACK_KNIGHTS_INDEX = 3;
inline char WHITE_BISHOPS_INDEX = 4;
inline char BLACK_BISHOPS_INDEX = 5;
inline char WHITE_ROOKS_INDEX = 6;
inline char BLACK_ROOKS_INDEX = 7;
inline char WHITE_QUEENS_INDEX = 8;
inline char BLACK_QUEENS_INDEX = 9;
inline char WHITE_KINGS_INDEX = 10;
inline char BLACK_KINGS_INDEX = 11;

inline int piece_values[10] = {100, // white pawn
               -100, // black pawn
                320, // white knight
               -320, // black knight
                330, // white bishop
               -330, // black bishop
                500, // white rook
               -500, // black rook
                900, // white queen
               -900};// black queen

inline int white_pawns_score[64] = 
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

inline int black_pawns_score[64] = 
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

inline int white_knights_score[64] = 
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

inline int black_knights_score[64] = 
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

inline int white_bishops_score[64] = 
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

inline int black_bishops_score[64] = 
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

inline int white_rooks_score[64] = 
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

inline int black_rooks_score[64] = 
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

inline int white_queens_score[64] = 
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

inline int black_queens_score[64] = 
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

inline int white_king_middlegame_score[64] = 
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

inline int black_king_middlegame_score[64] = 
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

inline int white_king_endgame_score[64] = 
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

inline int black_king_endgame_score[64] = 
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
inline int* piece_scores[14] = 
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
inline size_t SEARCH_TT_SIZE = 131072; // 2^17 THIS MUST BE A POWER OF 2
inline size_t EVAL_TT_SIZE = 131072; // 2^17 THIS MUST BE A POWER OF 2

inline int center_manhattan_distance_arr[64] = 
{
  6, 5, 4, 3, 3, 4, 5, 6,
  5, 4, 3, 2, 2, 3, 4, 5,
  4, 3, 2, 1, 1, 2, 3, 4,
  3, 2, 1, 0, 0, 1, 2, 3,
  3, 2, 1, 0, 0, 1, 2, 3,
  4, 3, 2, 1, 1, 2, 3, 4,
  5, 4, 3, 2, 2, 3, 4, 5,
  6, 5, 4, 3, 3, 4, 5, 6
};

inline int ENDGAME_MATERIAL = 2000;
inline int LAZY_EVAL_MARGIN = 200;
inline int ATTACKING_WEIGHT = 15;
inline int MOBILITY_WEIGHT = 1;

inline static uint64_t EN_PASSANT_SQ_MASK = 0x7F;
inline static uint16_t EN_PASSANT_OFFSET = 4;
inline static uint64_t PIECE_MASK = 0xF;
inline static uint16_t PIECE_OFFSET = 11;
inline static uint64_t FIFTY_MOVE_MASK = 0x7F;
inline static uint16_t FIFTY_MOVE_OFFSET = 15; 
inline static uint64_t IRR_PLY_MASK = 0x3FF;
inline static uint16_t IRR_PLY_OFFSET = 22; 
inline static uint64_t LAST_MOVE_MASK = 0xFFFFFFFF;
inline static uint16_t LAST_MOVE_OFFSET = 32;

inline int index64[64] = {
  0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

const unsigned long long debruijn64 = 0x03f79d71b4cb0a89;


}