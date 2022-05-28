/**
 * @file engine.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Provides an interface for the chess engine
 * @version 0.1
 * @date 2022-05-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// #include <cstdio>
// #include <cmath>
#include <string>
// #include <bitset>
#include <vector>
#include <stack>
// #include <time.h>
// #include <iostream>
// #include <fstream>
// #include <limits.h>
// #include <stdio.h>
// #include <cstring>

using namespace std;

typedef long long unsigned int bitboard;
typedef long long unsigned int uint64_t;
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

#define WHITE_PAWNS_INDEX 0x0
#define BLACK_PAWNS_INDEX 0x1
#define WHITE_KNIGHTS_INDEX 0x2
#define BLACK_KNIGHTS_INDEX 0x3
#define WHITE_BISHOPS_INDEX 0x4
#define BLACK_BISHOPS_INDEX 0x5
#define WHITE_ROOKS_INDEX 0x6
#define BLACK_ROOKS_INDEX 0x7
#define WHITE_QUEENS_INDEX 0x8
#define BLACK_QUEENS_INDEX 0x9
#define WHITE_KINGS_INDEX 0xA
#define BLACK_KINGS_INDEX 0xB

#define NO_CHECK 0x0
#define SINGLE_CHECK 0x1
#define DOUBLE_CHECK 0x2

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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
    bool white_king_side;
    bool white_queen_side;
    bool black_king_side;
    bool black_queen_side;

    // bitboard white_attack_map;
    // bitboard black_attack_map;

    square white_king_loc;
    square black_king_loc;
} board_t;

typedef struct LUTs {
    bitboard clear_rank[8];
    bitboard mask_rank[8];
    bitboard clear_file[8];
    bitboard mask_file[8];
    bitboard mask_diagonal[15]; // only 15 diagonals, 8th is for alignment
    bitboard mask_antidiagonal[15];
    bitboard pieces[64];

    bitboard king_attacks[64]; // these are currently unused
    bitboard white_pawn_attacks[64];
    bitboard black_pawn_attacks[64];
    bitboard white_pawn_pushes[64];
    bitboard black_pawn_pushes[64];
    bitboard knight_attacks[64];

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
    piece mv_piece;
    piece tar_piece;
    piece promotion_piece;
} move_t;

typedef struct pin_struct {
    bitboard ray_at_sq[64];
    bitboard pinned_pieces;
} pin_t;

/**
 * @brief Given a piece, it will return the index associated with that piece.
 * Each piece is mapped to an index 0 - 11 inclusive.
 * 
 * @param pc 
 * @return size_t
 */
size_t index_from_piece(piece pc);

/**
 * @brief Takes in a pointer to the current board state, along with a move
 * to be played. Makes a move on a copy of the input board, and returns
 * a pointer to the new board.
 * 
 * @param board 
 * @param move 
 * @return board_t* 
 */
board_t *make_move(board_t *board, move_t move);

/**
 * @brief Given a pointer to a stack of boards, it will undo the most
 * recent move.
 * 
 * @param boards 
 */
void unmake_move(stack<board_t *> *board_stack);

/**
 * @brief Given a valid FEN string, gives back a pointer to the board
 * which that FEN string represents.
 * 
 * @param fen 
 * @return board_t* 
 */
board_t *decode_fen(string fen);

/**
 * @brief Gives back the location of all pieces checking the friendly king.
 * Use in graphics to display the red check light on king.
 * @param board
 * @return Bitboard with all the pieces attacking the king.
 */
bitboard checking_pieces(board_t *board);

/**
 * @brief Given a move, a list of all legal moves, and the board state,
 * this function will return the standard chess notation representing that
 * unique move.
 * 
 * @param move The move to be played
 * @param all_moves Vector of all legal moves in the position
 * @param board Current board state
 * @return Standard chess notation for input move. 
 */
string notation_from_move(move_t move, vector<move_t> all_moves, board_t *board);

// Include best move function for interacting with the engine

void generate_moves(board_t *board, vector<move_t> *curr_moves);

move_t find_best_move(board_t *board, size_t depth);