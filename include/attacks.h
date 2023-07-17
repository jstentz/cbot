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


// after doing magic bitboards, all of the getters will be inline
class LookUpTable
{
public:
  LookUpTable(); // construct the lut

  inline bitboard get_knight_attacks(int sq) const
  {
    // doesn't depend on the current position
    return knight_attacks[sq];
  }

  inline bitboard get_king_attacks(int sq) const
  {
    // doesn't depend on the current position
    return king_attacks[sq];
  }

  inline bitboard get_pawn_attacks(int sq, bool white_side) const
  {
    // depends on who's turn it is to move
    if(white_side) return white_pawn_attacks[sq];
    return black_pawn_attacks[sq];
  }
  
  inline bitboard get_pawn_pushes(int sq, bool white) const
  {
    return white ? white_pawn_pushes[sq] : black_pawn_pushes[sq];
  }

  bitboard get_rook_attacks(int sq, bitboard blockers) const;
  bitboard get_bishop_attacks(int sq, bitboard blockers) const;
  bitboard get_queen_attacks(int sq, bitboard blockers) const;

  bitboard get_ray_from_bishop_to_king(int bishop_sq, int king_sq) const;
  bitboard get_ray_from_rook_to_king(int rook_sq, int king_sq) const;
  bitboard get_ray_from_queen_to_king(int queen_sq, int king_sq) const;
  bitboard get_ray_from_sq_to_sq(int start_sq, int target_sq) const;

  inline bitboard get_rank_mask(int rank) const
  {
    return mask_rank[rank];
  }

  

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