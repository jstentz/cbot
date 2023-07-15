#include "include/hashing.h"
#include "include/pieces.h"
#include "include/constants.h"
#include "include/utils.h"

#include <cstdlib>
#include <time.h>
#include <stdio.h>

Hasher::Hasher()
{
  srand(1234124);
  /** 
   * IMPORTANT I DON'T RESEED THIS EVERYTIME 
   * IM NOT SURE IF THIS COULD MEAN THAT THERE WOULD BE ISSUES WHEN HASHING BOARDS
   * THE SAME BOARD WILL ALWAYS HASH TO THE SAME NUMBER ACROSS PROGRAM LAUNCHES
   * ... I don't think this matters though because that just means that if there
   * is a conflict (which there shouldn't be) it'll always be the same boards
   * conflicting
   */
  for (int i = 0; i < 64; i++) 
  {
    for (int j = 0; j < 12; j++) 
    {
      m_zobrist_table.table[i][j] = utils::rand64();
    }
  }

  m_zobrist_table.black_to_move = utils::rand64();
  m_zobrist_table.white_king_side = utils::rand64();
  m_zobrist_table.white_queen_side = utils::rand64();
  m_zobrist_table.black_king_side = utils::rand64();
  m_zobrist_table.black_queen_side = utils::rand64();

  for (int i = 0; i < 8; i++) 
  {
    m_zobrist_table.en_passant_file[i] = utils::rand64();
  }
}

uint64_t Hasher::hash_board(bool white_turn, piece* sq_board, bool white_ks, bool white_qs, bool black_ks, bool black_qs, int en_passant_sq) const
{
  uint64_t h = hash_pieces(sq_board);
  if (!white_turn) h ^= m_zobrist_table.black_to_move;

  if (white_ks) h ^= m_zobrist_table.white_king_side;
  if (white_qs) h ^= m_zobrist_table.white_queen_side;
  if (black_ks) h ^= m_zobrist_table.black_king_side;
  if (black_qs) h ^= m_zobrist_table.black_queen_side;

  if (en_passant_sq != constants::NONE) 
  {
    h ^= m_zobrist_table.en_passant_file[utils::file(en_passant_sq)];
  }
  return h;
}

uint64_t Hasher::hash_pieces(piece* sq_board) const
{
  uint64_t h = 0;
  for (int i = 0; i < 64; i++) 
  {
    piece pc = sq_board[i];
    if (pc != EMPTY) 
    {
      size_t j = utils::index_from_pc(pc);
      h ^= m_zobrist_table.table[i][j];
    }
  }
}

uint64_t Hasher::hash_pawns(piece* sq_board) const
{
  uint64_t h = 0;
  for (int i = 0; i < 64; i++) 
  {
    piece pc = sq_board[i];
    if (PIECE(pc) == PAWN) 
    {
      h ^= m_zobrist_table.table[i][utils::index_from_pc(pc)];
    }
  }
  return h;
}
