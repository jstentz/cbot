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

lut_t init_LUT ();

extern lut_t luts;

bitboard get_knight_attacks(square knight);

bitboard get_king_attacks(square king);

bitboard get_pawn_attacks(square pawn, turn side);

bitboard get_rook_attacks(square rook, bitboard all_pieces);

bitboard get_bishop_attacks(square bishop, bitboard all_pieces);

bitboard get_queen_attacks(square queen, bitboard all_pieces);

bitboard generate_attack_map(board_t board, turn side);

bitboard get_ray_from_bishop_to_king(square bishop, square king);

bitboard get_ray_from_rook_to_king(square rook, square king);

bitboard get_ray_from_queen_to_king(square queen, square king);

bitboard get_ray_from_sq_to_sq(square start, square target);

bool is_attacked(board_t *board, square sq, bitboard blocking_pieces);

bitboard attackers_from_square(board_t *board, square sq);

bitboard opponent_slider_rays_to_square(board_t *board, square sq);