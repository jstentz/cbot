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

#include "include/bitboard.h"

class LookUpTable
{
public:
  LookUpTable(); // construct the lut

  bitboard get_knight_attacks(int sq) const;
  bitboard get_king_attacks(int sq) const;
  bitboard get_pawn_attacks(int sq, bool white_side) const;
  bitboard get_pawn_pushes(int sq, bool white) const;
  bitboard get_rook_attacks(int sq, bitboard blockers) const;
  bitboard get_bishop_attacks(int sq, bitboard blockers) const;
  bitboard get_queen_attacks(int sq, bitboard blockers) const;

  bitboard get_ray_from_bishop_to_king(int bishop_sq, int king_sq) const;
  bitboard get_ray_from_rook_to_king(int rook_sq, int king_sq) const;
  bitboard get_ray_from_queen_to_king(int queen_sq, int king_sq) const;
  bitboard get_ray_from_sq_to_sq(int start_sq, int target_sq) const;

  bitboard get_rank_mask(int rank) const;
  

private:
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
};