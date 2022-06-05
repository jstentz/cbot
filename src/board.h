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

#include "bitboard.h"
#include "pieces.h"
#include "hashing.h"

using namespace std;

typedef bool turn;

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
    square en_passant;
    bool white_king_side;
    bool white_queen_side;
    bool black_king_side;
    bool black_queen_side;

    square white_king_loc;
    square black_king_loc;

    /* evaluation items */
    int total_material;
    int material_score; // this will always be from white's perspective
    int piece_placement_score; // score based on piece locations

    /* hashing items */
    hash_val board_hash;
} board_t;

/**
 * @brief Struct containing the state of pins on the board
 * 
 */
typedef struct pin_struct {
    bitboard ray_at_sq[64];
    bitboard pinned_pieces;
} pin_t;

/**
 * @brief Sets a bit on the given bitboard at the given square.
 * 
 * @param bb Bitboard
 * @param sq Location of piece
 */
void place_piece(bitboard *bb, square sq);

/**
 * @brief Removes a bit on the given bitboard at the given square.
 * 
 * @param bb Bitboard
 * @param sq Location of piece
 */
void rem_piece(bitboard *bb, square sq);

/**
 * @brief Given a board, it makes sure that the extra boards, such as all_pieces
 * are updated with the piece bitboards.
 * 
 * @param board Board to update
 */
void update_boards(board_t *board);

/**
 * @brief Given a valid FEN string, gives back a pointer to the board
 * which that FEN string represents.
 * 
 * @param fen FEN-style string
 * @return Pointer to board represented by FEN string
 */
board_t *decode_fen(string fen);

/**
 * @brief Given a board state and the location of the friendly king, it
 * returns a struct outlining the pins on the board.
 * 
 * @param board Current board state
 * @param friendly_king_loc 
 * @return pin_t 
 */
pin_t get_pinned_pieces(board_t *board, square friendly_king_loc);

/**
 * @brief Given a board state, it returns a bitboard with the location of the
 * pinned pieces.
 * 
 * @param board Current board state
 * @return Bitboard containing location of pinned pieces 
 */
bitboard checking_pieces(board_t *board);

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
bitboard checking_pieces(board_t *board);