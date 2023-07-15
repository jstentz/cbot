/**
 * @file hashing.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines the necessary types and functions for hashing boards
 * @version 0.1
 * @date 2022-06-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <cstdint>

#include "include/pieces.h"

class Hasher
{
public:
  /// @brief initializes the internal table 
  Hasher();
  ~Hasher() {}

  uint64_t hash_board(bool white_turn, piece* sq_board, bool white_ks, bool white_qs, bool black_ks, bool black_qs, int en_passant_sq) const;
  uint64_t hash_pieces(piece* sq_board) const;
  uint64_t hash_pawns(piece* sq_board) const;

  uint64_t get_hash_val(piece pc, int sq) const;
  uint64_t get_white_king_side_hash() const;
  uint64_t get_white_queen_side_hash() const;
  uint64_t get_black_king_side_hash() const;
  uint64_t get_black_queen_side_hash() const;
  uint64_t get_en_passant_hash(int sq) const;
  uint64_t get_black_to_move_hash() const;
private:
  struct ZobristTable 
  {
  uint64_t table[64][12];
  uint64_t black_to_move;
  uint64_t white_king_side;
  uint64_t black_king_side;
  uint64_t white_queen_side;
  uint64_t black_queen_side;

  uint64_t en_passant_file[8];
  } m_zobrist_table;
};