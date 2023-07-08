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
#include <vector>
#include <memory>

#include "include/bitboard.h"
#include "include/pieces.h"
#include "include/constants.h"

typedef signed int move_t;

// #define FILE(sq) (sq & 7)
// #define RANK(sq) (sq >> 3)

// #define DIAG(sq)        (7 + RANK(sq) - FILE(sq))
// #define ANTI_DIAG(sq)   (RANK(sq) + FILE(sq))

// #define PLACE_PIECE(bb, sq) (bb = (bb | (((bitboard)0x1) << sq)))
// #define REM_PIECE(bb, sq) (bb = (bb & ~(((bitboard)0x1) << sq)))


class Board
{
public:
  using Ptr = std::shared_ptr<Board>;
  using ConstPtr = std::shared_ptr<const Board>;

  Board(std::string fen);
  
  /* types */
  /// TODO: add other types of draws here (50 move rule)
  enum class BoardStatus 
  {
    ONGOING,
    WHITE_WIN,
    BLACK_WIN,
    STALEMATE,
    REPETITION
  };

  enum class Square { A1, B1, C1, D1, E1, F1, G1, H1,
                      A2, B2, C2, D2, E2, F2, G2, H2,
                      A3, B3, C3, D3, E3, F3, G3, H3,
                      A4, B4, C4, D4, E4, F4, G4, H4,
                      A5, B5, C5, D5, E5, F5, G5, H5,
                      A6, B6, C6, D6, E6, F6, G6, H6,
                      A7, B7, C7, D7, E7, F7, G7, H7,
                      A8, B8, C8, D8, E8, F8, G8, H8, NONE };

  enum class Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
  enum class File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

  enum class CheckType { NONE, SINGLE, DOUBLE };

  /**
   * @brief Resets to starting position 
  */
  void reset();

  /**
   * @brief Resets to a given fen string 
   * @param[in] fen position to reset to 
  */
  void reset(std::string fen);
  
  void make_move(move_t move);
  void unmake_move(move_t move);

  void make_moves(move_t move);
  void unmake_moves(move_t move);

  /**
   * @brief Generates the legal moves for current state
   * @param[out] moves vector of moves to populate
  */
  void generate_moves(std::vector<move_t> moves) const;

  BoardStatus get_game_status() const;

  bool is_white_turn() const;

  std::string to_string() const;
  std::string to_fen_string() const;

private:

  /**
   * @brief Holds the irreversible state information 
   * @details
   * State bit breakdown from LSB to MSB
   * Bits 0 - 3: castling rights KQkq
   * Bits 4 - 10: en passant square (64 here means no square)
   * Bits 11 - 14: last captured piece (0000 means EMPTY)
   * Bits 15 - 31: 50 move counter
   * Bits 32 - 63: move played to reach this position (used for recapture move ordering)
   */
  class IrreversibleState
  {
  public:
    IrreversibleState(bool white_ks, 
                      bool white_qs, 
                      bool black_ks, 
                      bool black_qs, 
                      Square en_passant_sq, 
                      piece last_capture, 
                      uint16_t fifty_move_count,
                      uint32_t irr_ply,
                      move_t last_move);
    
    /* getters */
    inline bool can_white_king_side_castle() const  { return m_state & (1 << 3); }
    inline bool can_white_queen_side_castle() const { return m_state & (1 << 2); }
    inline bool can_black_king_side_castle() const  { return m_state & (1 << 1); }
    inline bool can_black_queen_side_castle() const { return m_state & (1 << 0); }
    inline bool can_white_castle() const            { return can_white_king_side_castle() || can_white_queen_side_castle(); }
    inline bool can_black_castle() const            { return can_black_king_side_castle() || can_black_queen_side_castle(); }

    inline Square get_en_passant_sq() const { return (Square)((m_state >> EN_PASSANT_OFFSET) & EN_PASSANT_SQ_MASK); }
    inline piece get_last_capture() const   { return (piece)((m_state >> PIECE_OFFSET) & PIECE_MASK); }
    inline uint16_t get_fifty_move() const  { return (m_state >> FIFTY_MOVE_OFFSET) & FIFTY_MOVE_MASK; }
    inline uint32_t get_irr_ply() const     { return (m_state >> IRR_PLY_OFFSET) & IRR_PLY_MASK; }
    inline move_t get_last_move() const     { return (m_state >> LAST_MOVE_OFFSET) & LAST_MOVE_MASK; }

    /* setters */
    inline void set_white_king_side_castle(bool can_castle)  { m_state &= ~(1 << 3); m_state |= can_castle << 3; }
    inline void set_white_queen_side_castle(bool can_castle) { m_state &= ~(1 << 2); m_state |= can_castle << 2; }
    inline void set_black_king_side_castle(bool can_castle)  { m_state &= ~(1 << 1); m_state |= can_castle << 1; }
    inline void set_black_queen_side_castle(bool can_castle) { m_state &= ~(1 << 0); m_state |= can_castle << 0; }
    inline void set_white_castle(bool can_castle)            { set_white_king_side_castle(can_castle); set_white_queen_side_castle(can_castle); }
    inline void set_black_castle(bool can_castle)            { set_black_king_side_castle(can_castle); set_black_queen_side_castle(can_castle); }

    inline void clr_en_passant_sq()           { m_state &= ~(EN_PASSANT_SQ_MASK << EN_PASSANT_OFFSET); }
    inline void set_en_passant_sq(Square sq)  { clr_en_passant_sq(); m_state |= ((uint64_t) sq & EN_PASSANT_SQ_MASK) << EN_PASSANT_OFFSET; }

    inline void clr_last_capture()          { m_state &= ~(PIECE_MASK << PIECE_OFFSET); }
    inline void set_last_capture(piece pc)  { clr_last_capture(); m_state |= ((uint64_t) pc & PIECE_MASK) << PIECE_OFFSET; }

    inline void clr_fifty_move()                    { m_state &= ~(FIFTY_MOVE_MASK << FIFTY_MOVE_OFFSET); }
    inline void set_fifty_move(uint16_t move_count) { clr_fifty_move(); m_state |= (move_count & FIFTY_MOVE_MASK) << FIFTY_MOVE_OFFSET; }
    inline void inc_fifty_move()                    { set_fifty_move(get_fifty_move() + 1); }

    inline void clr_irr_ply()             { m_state &= ~(IRR_PLY_MASK << IRR_PLY_OFFSET); }
    inline void set_irr_ply(uint32_t ply) { clr_irr_ply(); m_state |= (ply & IRR_PLY_MASK) << IRR_PLY_OFFSET; }

    inline void clr_last_move()                 { m_state &= ~(LAST_MOVE_MASK << LAST_MOVE_OFFSET); }
    inline void set_last_move(move_t last_move) { clr_last_move(); m_state |= (last_move & LAST_MOVE_MASK) << LAST_MOVE_OFFSET; }

  private:
    uint64_t m_state{};
    const static uint64_t EN_PASSANT_SQ_MASK = 0x7F;
    const static uint16_t EN_PASSANT_OFFSET = 4;
    const static uint64_t PIECE_MASK = 0xF;
    const static uint16_t PIECE_OFFSET = 11;
    const static uint64_t FIFTY_MOVE_MASK = 0x7F;
    const static uint16_t FIFTY_MOVE_OFFSET = 15; 
    const static uint64_t IRR_PLY_MASK = 0x3FF;
    const static uint16_t IRR_PLY_OFFSET = 22; 
    const static uint64_t LAST_MOVE_MASK = 0xFFFFFFFF;
    const static uint16_t LAST_MOVE_OFFSET = 32;
  };

  /**
   * @brief Holds information regarding a pin
  */
  struct Pin {
    bitboard ray_at_sq[64];
    bitboard pinned_pieces{};
  };

  /**
   * @brief Updates the redudant board representations
  */
  void update_redundant_boards();

  int check_type(bitboard attackers) const;
  Pin get_pinned_pieces(Square friendly_king_loc) const;
  bitboard checking_pieces() const;
  int check_type(bitboard attackers) const;
  bool in_check() const;
  bool is_repetition() const;
  int file(Square sq) const;
  int rank(Square sq) const;
  int diag(Square sq) const;
  int anti_diag(Square sq) const;

  void place_piece_in_bb(piece pc, Square sq) const;
  void rem_piece_from_bb(piece pc, Square sq) const;

  bitboard bb_from_piece(piece pc) const;

  Square name_to_sq(std::string sq_name) const;

  /* private members */

  bitboard m_piece_boards[12];
  bitboard m_white_pieces;
  bitboard m_black_pieces;
  bitboard m_all_pieces;

  piece m_sq_board[64];

  bool m_white_turn;

  Square m_white_king_loc;
  Square m_black_king_loc;

  /* hashing items */
  uint64_t m_board_hash;
  uint64_t m_piece_hash;
  uint64_t m_pawn_hash;

  /* evaluation items */
  int m_material_score;
  int m_positional_score; // doesn't include kings
  int m_piece_counts[10];
  int m_total_material;

  std::stack<IrreversibleState> m_state_history;

  /* for detecting repetition */
  /* I have to encode the ply of the last irreversible in the state */
  int m_last_irreveresible_move_ply;
  std::vector<uint64_t> m_board_hash_history;

  /* internal constants */
  static const char WHITE_PAWNS_INDEX = 0;
  static const char BLACK_PAWNS_INDEX = 1;
  static const char WHITE_KNIGHTS_INDEX = 2;
  static const char BLACK_KNIGHTS_INDEX = 3;
  static const char WHITE_BISHOPS_INDEX = 4;
  static const char BLACK_BISHOPS_INDEX = 5;
  static const char WHITE_ROOKS_INDEX = 6;
  static const char BLACK_ROOKS_INDEX = 7;
  static const char WHITE_QUEENS_INDEX = 8;
  static const char BLACK_QUEENS_INDEX = 9;
  static const char WHITE_KINGS_INDEX = 10;
  static const char BLACK_KINGS_INDEX = 11;
  
};
