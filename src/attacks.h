/**
 * @file attacks.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines functions for generating attacks
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "bitboard.h"
#include "board.h"
/**
 * @brief Structure containing all precalculated data
 * 
 */
typedef struct LUTs {
    bitboard clear_rank[8];
    bitboard mask_rank[8];
    bitboard clear_file[8];
    bitboard mask_file[8];
    bitboard mask_diagonal[15]; // only 15 diagonals, 8th is for alignment
    bitboard mask_antidiagonal[15];
    bitboard pieces[64];

    bitboard king_attacks[64]; 
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

/**
 * @brief Initializes all precalculated data.
 * 
 * @return Lookup table containing precalculated data
 */
lut_t init_LUT ();

/**
 * @brief Global structure containing all precalculated data
 * 
 */
extern lut_t luts;

/**
 * @brief Gives knight attacks from a knight's location.
 * 
 * @param knight location of knight
 * @return Bitboard containing squares attacked by knight
 */
bitboard get_knight_attacks(square knight);

/**
 * @brief Gives king attacks from a king's location.
 * 
 * @param king location of king
 * @return Bitboard containing squares attacked by king
 */
bitboard get_king_attacks(square king);

/**
 * @brief Gives pawn attacks from a pawn's location.
 * 
 * @param pawn location of pawn
 * @return Bitboard containing squares attacked by pawn
 */
bitboard get_pawn_attacks(square pawn, turn side);

/**
 * @brief Gives rook attacks from a rook's location.
 * 
 * @param rook location of rook
 * @return Bitboard containing squares attacked by rook
 */
bitboard get_rook_attacks(square rook, bitboard all_pieces);

/**
 * @brief Gives bishop attacks from a bishop's location.
 * 
 * @param bishop location of bishop
 * @return Bitboard containing squares attacked by bishop
 */
bitboard get_bishop_attacks(square bishop, bitboard all_pieces);

/**
 * @brief Gives queen attacks from a queen's location.
 * 
 * @param queen location of queen
 * @return Bitboard containing squares attacked by queen
 */
bitboard get_queen_attacks(square queen, bitboard all_pieces);

/**
 * @brief Generates an attack map from a side's perspective.
 * 
 * @param board board state
 * @param side side to get pieces attacks from
 * @return Bitboard containing attacked squares by side
 */
bitboard generate_attack_map(board_t board, turn side);

/**
 * @brief Given the location of a bishop and a king, gives back the ray 
 * from the bishop to the king (including the bishop and king).
 * 
 * @param bishop location of bishop
 * @param king location of king
 * @return Bitboard containing the ray
 */
bitboard get_ray_from_bishop_to_king(square bishop, square king);

/**
 * @brief Given the location of a rook and a king, gives back the ray 
 * from the rook to the king (including the rook and king).
 * 
 * @param rook location of rook
 * @param king location of king
 * @return Bitboard containing the ray
 */
bitboard get_ray_from_rook_to_king(square rook, square king);

/**
 * @brief Given the location of a queen and a king, gives back the ray 
 * from the queen to the king (including the queen and king).
 * 
 * @param queen location of queen
 * @param king location of king
 * @return Bitboard containing the ray
 */
bitboard get_ray_from_queen_to_king(square queen, square king);

/**
 * @brief Given the two squares, gives back the ray between the two squares
 *  (including the two squares).
 * 
 * @param start start square location
 * @param target target square location
 * @return Bitboard containing the ray
 */
bitboard get_ray_from_sq_to_sq(square start, square target);

/**
 * @brief Given the board state and a square and the pieces on the board that
 * should be considered as blocking the attack.
 * 
 * @param board Current board state
 * @param sq Location of square
 * @param blocking_pieces All the pieces on the board to be considered as blocking the attack
 * @return Whether or not the square is attacked
 */
bool is_attacked(board_t *board, square sq, bitboard blocking_pieces);

/**
 * @brief Given the board state and a square, it returns a bitboard containing
 * the attackers of that square.
 * 
 * @param board Current board state
 * @param sq Location of square
 * @return Bitboard of all the attackers of the square
 */
bitboard attackers_from_square(board_t *board, square sq);

/**
 * @brief Given the board state and a square, it returns a bitboard containing
 * the opponent's sliding rays to that square.
 * 
 * @param board Current board state
 * @param sq Location of square
 * @return Bitboard containing all the rays
 */
bitboard opponent_slider_rays_to_square(board_t *board, square sq);