/**
 * @file moves.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Defines moves and functions to generate and interact with them
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <vector>
#include <string>

#include "include/move.h"
#include "include/board.h"
#include "include/attacks.h"

class MoveGenerator
{
public:
  MoveGenerator(const Board& board);

  void generate_moves(std::vector<Move> &curr_moves, bool captures_only = false) const;
  void order_moves(std::vector<Move> &moves, Move tt_best_move) const;


  std::string notation_from_move(Move move) const;
  Move move_from_notation(std::string notation) const;
  std::string move_to_long_algebraic(Move move) const;
  Move long_algebraic_to_move(std::string notation) const;
  void sort_by_long_algebraic_notation(std::vector<Move> &moves) const;

private:
  const Board& m_board;
  const static LookUpTable lut; // initialized on program start

  struct Pin {
    bitboard ray_at_sq[64];
    bitboard pinned_pieces{};
  };

  enum class CheckType { NONE, SINGLE, DOUBLE };

  Pin get_pinned_pieces(int friendly_king_loc) const;

  bitboard checking_pieces() const;
  CheckType check_type(bitboard checkers) const;
  bool in_check() const;

  int see(int sq) const;
  int see_capture(Move capture) const;
  bool is_bad_capture(Move capture) const;
  bool pawn_promo_or_close_push(Move move) const;

  bitboard generate_knight_move_bitboard(int knight_sq, bool captures_only = false) const;
  bitboard generate_king_move_bitboard(int king_sq, bool captures_only = false) const;
  bitboard generate_pawn_move_bitboard(int pawn_sq, bool captures_only = false) const;
  bitboard generate_rook_move_bitboard(int rook_sq, bool captures_only = false) const;
  bitboard generate_bishop_move_bitboard(int bishop_sq, bool captures_only = false) const;
  bitboard generate_queen_move_bitboard(int queen_sq, bool captures_only = false) const;

  void generate_king_moves(std::vector<Move> &curr_moves, bool captures_only = false) const;
  void generate_knight_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only = false) const;
  void generate_pawn_moves(std::vector<Move> &curr_moves, bitboard check_mask, bool pawn_check, Pin &pin, bool captures_only = false) const;
  void generate_rook_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only = false) const;
  void generate_bishop_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only = false) const;
  void generate_queen_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only = false) const;

  bitboard generate_attack_map(bool white_side) const;
  piece least_valued_attacker(int sq) const;
  int least_valued_attacker_sq(int sq, bool white_turn) const;
  bool is_attacked_by_pawn(int sq) const;
  bool is_attacked(int sq, bitboard blockers) const;
  bitboard attackers_from_square(int sq) const;
  bitboard opponent_slider_rays_to_square(int sq) const;
};