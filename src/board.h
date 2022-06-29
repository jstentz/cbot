/**
 * @file board.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Provides board representation and functions to interact with it
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <iostream>
#include <stdio.h>
#include <string>
#include <stack>

#include "bitboard.h"
#include "pieces.h"

using namespace std;

typedef bool turn;
typedef signed int move_t;

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

#define W (turn)true
#define B (turn)false

#define FILE(sq) (sq & 7)
#define RANK(sq) (sq >> 3)

#define NO_CHECK 0x0
#define SINGLE_CHECK 0x1
#define DOUBLE_CHECK 0x2

#define PLACE_PIECE(bb, sq) (bb = (bb | (((bitboard)0x1) << sq)))
#define REM_PIECE(bb, sq) (bb = (bb & ~(((bitboard)0x1) << sq)))

#define NO_MOVE ((move_t)0x0)

/* defines all operations on the board state number */
#define WHITE_CASTLE(state)     (state & 0x000000000000000C)
#define WHITE_KING_SIDE(state)  (state & 0x0000000000000008)
#define WHITE_QUEEN_SIDE(state) (state & 0x0000000000000004)
#define BLACK_CASTLE(state)     (state & 0x0000000000000003)
#define BLACK_KING_SIDE(state)  (state & 0x0000000000000002)
#define BLACK_QUEEN_SIDE(state) (state & 0x0000000000000001)

#define SET_WHITE_KING_SIDE(state)  (state |= 0x0000000000000008)
#define SET_WHITE_QUEEN_SIDE(state) (state |= 0x0000000000000004)
#define SET_BLACK_KING_SIDE(state)  (state |= 0x0000000000000002)
#define SET_BLACK_QUEEN_SIDE(state) (state |= 0x0000000000000001)

#define REM_WHITE_KING_SIDE(state)  (state &= ~0x0000000000000008)
#define REM_WHITE_QUEEN_SIDE(state) (state &= ~0x0000000000000004)
#define REM_BLACK_KING_SIDE(state)  (state &= ~0x0000000000000002)
#define REM_BLACK_QUEEN_SIDE(state) (state &= ~0x0000000000000001)

#define EN_PASSANT_SQ(state)         ((state >> 4) & 0x000000000000003F)
#define SET_EN_PASSANT_SQ(state, sq) (state |= (sq << 4))

#define LAST_CAPTURE(state)         ((state >> 10) & 0x000000000000000F)
#define SET_LAST_CAPTURE(state, pc) (state |= (pc << 10))

#define FIFTY_MOVE(state)            ((state >> 14) & 0x000000000003FFFF)
#define SET_FIFTY_MOVE(state, count) (state |= (count << 14))
#define INC_FIFTY_MOVE(state)        (SET_FIFTY_MOVE(state, FIFTY_MOVE(state) + 1))
#define CL_FIFTY_MOVE(state)         (state &= (~0x000000000003FFFF) << 14)

#define LAST_MOVE(state)         ((state >> 32) & 0x00000000FFFFFFFF)
#define SET_LAST_MOVE(state, mv) (state |= (mv << 32))


// would like to get rid of these enums in any non-user interacting code
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

typedef unsigned long long int hash_val; // stolen from hashing.h
// NOT THE FIX I LIKE

/**
 * State bit breakdown from LSB to MSB
 * Bits 0 - 3: castling rights KQkq
 * Bits 4 - 9: en passant square (64 here means no square)
 * Bits 10 - 13: last captured piece (0000 means EMPTY)
 * Bits 14 - 31: 50 move counter
 * Bits 32 - 63: move played to reach this position (used for recapture move ordering)
 */
typedef unsigned long long int state_t;

/**
 * @brief Struct containing all board state data
 * 
 */
typedef struct Board
{
    bitboard piece_boards[12];

    bitboard white_pieces;
    bitboard black_pieces;
    bitboard all_pieces;

    piece sq_board[64];

    turn t;

    square white_king_loc;
    square black_king_loc;

    /* hashing items */
    hash_val board_hash;

    /* evaluation items */
    int material_score;
    int positional_score; // doesn't include kings
    int piece_counts[10];

    stack<state_t> state_history;
} board_t;

extern board_t b;

/**
 * @brief Struct containing the state of pins on the board
 * 
 */
typedef struct pin_struct {
    bitboard ray_at_sq[64];
    bitboard pinned_pieces;
} pin_t;

/**
 * @brief Given a board, it makes sure that the extra boards, such as all_pieces
 * are updated with the piece bitboards.
 * 
 * @param board Board to update
 */
void update_boards();

/**
 * @brief Given a valid FEN string, gives back a pointer to the board
 * which that FEN string represents.
 * 
 * @param fen FEN-style string
 * @return Board represented by FEN string
 */
board_t decode_fen(string fen);

/**
 * @brief Given a board state, gives back a FEN string representing
 * that board state.
 * 
 * @param board Board state
 * @return FEN string
 */
// string encode_fen(board_t *board);

/**
 * @brief Given a board state and the location of the friendly king, it
 * returns a struct outlining the pins on the board.
 * 
 * @param board Current board state
 * @param friendly_king_loc 
 * @return pin_t 
 */
pin_t get_pinned_pieces(square friendly_king_loc);

/**
 * @brief Given a board state, it returns a bitboard with the location of the
 * pinned pieces.
 * 
 * @param board Current board state
 * @return Bitboard containing location of pinned pieces 
 */
bitboard checking_pieces();

/**
 * @brief Given attackers of the king, it returns the type of check we are in.
 * 
 * @param attackers Pieces attacking the king
 * @return Double check, single check, or no check 
 */
int in_check(bitboard attackers);

/**
 * @brief Gives back the location of all pieces checking the friendly king.
 * Use in graphics to display the red check light on king.
 * @param board
 * @return Bitboard with all the pieces attacking the king.
 */
bitboard checking_pieces();