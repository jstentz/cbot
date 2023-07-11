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
#include <vector>
#include <memory>

#include "include/bitboard.h"
#include "include/pieces.h"
#include "include/constants.h"
#include "include/move.h"

class Board
{
public:
  using Ptr = std::shared_ptr<Board>;
  using ConstPtr = std::shared_ptr<const Board>;

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

  enum class CheckType { NO_CHECK, SINGLE, DOUBLE };

  Board();
  Board(std::string fen);

  /**
   * @brief Resets to starting position 
  */
  void reset();

  /**
   * @brief Resets to a given fen string 
   * @param[in] fen position to reset to 
  */
  void reset(std::string fen);

  void clear();
  
  void make_move(Move move);
  void unmake_move(Move move);

  void make_nullmove();
  void unmake_nullmove();

  // useful for uci implementation 
  void make_moves(std::vector<Move> moves);
  // void unmake_moves(std::vector<move_t> moves);

  BoardStatus get_game_status() const;

  bool is_white_turn() const;

  // *IMPORTANT* for all of these getters, consider using a const reference for speed reasons...

  // indexing into the square board
  piece operator[](int sq) const;

  int get_piece_count(piece pc) const;
  int get_material_score() const;
  int get_positional_score() const;
  int get_total_material() const;

  uint64_t get_hash() const;
  uint64_t get_piece_hash() const;
  uint64_t get_pawn_hash() const;
  bool is_repetition() const;

  bitboard get_piece_bitboard(piece pc) const;
  int get_white_king_loc() const;
  int get_black_king_loc() const;
  bool can_white_king_side_castle() const;
  bool can_white_queen_side_castle() const;
  bool can_black_king_side_castle() const;
  bool can_black_queen_side_castle() const;
  int get_en_passant_sq() const;
  Move get_last_move() const;

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
                      int en_passant_sq, 
                      piece last_capture, 
                      uint16_t fifty_move_count,
                      uint32_t irr_ply,
                      Move last_move);
    
    /* getters */
    inline bool can_white_king_side_castle() const  { return m_state & (1 << 3); }
    inline bool can_white_queen_side_castle() const { return m_state & (1 << 2); }
    inline bool can_black_king_side_castle() const  { return m_state & (1 << 1); }
    inline bool can_black_queen_side_castle() const { return m_state & (1 << 0); }
    inline bool can_white_castle() const            { return can_white_king_side_castle() || can_white_queen_side_castle(); }
    inline bool can_black_castle() const            { return can_black_king_side_castle() || can_black_queen_side_castle(); }

    inline int get_en_passant_sq() const    { return (m_state >> EN_PASSANT_OFFSET) & EN_PASSANT_SQ_MASK; }
    inline piece get_last_capture() const   { return (m_state >> PIECE_OFFSET) & PIECE_MASK; }
    inline uint16_t get_fifty_move() const  { return (m_state >> FIFTY_MOVE_OFFSET) & FIFTY_MOVE_MASK; }
    inline uint32_t get_irr_ply() const     { return (m_state >> IRR_PLY_OFFSET) & IRR_PLY_MASK; }
    inline Move get_last_move() const       { return Move((m_state >> LAST_MOVE_OFFSET) & LAST_MOVE_MASK); }

    /* setters */
    inline void set_white_king_side_castle(bool can_castle)  { m_state &= ~(1 << 3); m_state |= can_castle << 3; }
    inline void set_white_queen_side_castle(bool can_castle) { m_state &= ~(1 << 2); m_state |= can_castle << 2; }
    inline void set_black_king_side_castle(bool can_castle)  { m_state &= ~(1 << 1); m_state |= can_castle << 1; }
    inline void set_black_queen_side_castle(bool can_castle) { m_state &= ~(1 << 0); m_state |= can_castle << 0; }
    inline void set_white_castle(bool can_castle)            { set_white_king_side_castle(can_castle); set_white_queen_side_castle(can_castle); }
    inline void set_black_castle(bool can_castle)            { set_black_king_side_castle(can_castle); set_black_queen_side_castle(can_castle); }

    inline void clr_en_passant_sq()        { m_state &= ~(EN_PASSANT_SQ_MASK << EN_PASSANT_OFFSET); }
    inline void set_en_passant_sq(int sq)  { clr_en_passant_sq(); m_state |= ((uint64_t) sq & EN_PASSANT_SQ_MASK) << EN_PASSANT_OFFSET; }

    inline void clr_last_capture()          { m_state &= ~(PIECE_MASK << PIECE_OFFSET); }
    inline void set_last_capture(piece pc)  { clr_last_capture(); m_state |= ((uint64_t) pc & PIECE_MASK) << PIECE_OFFSET; }

    inline void clr_fifty_move()                    { m_state &= ~(FIFTY_MOVE_MASK << FIFTY_MOVE_OFFSET); }
    inline void set_fifty_move(uint16_t move_count) { clr_fifty_move(); m_state |= (move_count & FIFTY_MOVE_MASK) << FIFTY_MOVE_OFFSET; }
    inline void inc_fifty_move()                    { set_fifty_move(get_fifty_move() + 1); }

    inline void clr_irr_ply()             { m_state &= ~(IRR_PLY_MASK << IRR_PLY_OFFSET); }
    inline void set_irr_ply(uint32_t ply) { clr_irr_ply(); m_state |= (ply & IRR_PLY_MASK) << IRR_PLY_OFFSET; }

    inline void clr_last_move()               { m_state &= ~(LAST_MOVE_MASK << LAST_MOVE_OFFSET); }
    inline void set_last_move(Move last_move) { clr_last_move(); m_state |= (last_move.get_move() & LAST_MOVE_MASK) << LAST_MOVE_OFFSET; }

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
   * @brief Updates the redudant board representations
  */
  void update_redundant_boards();

  void place_piece_in_bb(piece pc, int sq);
  void remove_piece_from_bb(piece pc, int sq);

  void place_piece(piece pc, int sq);
  void remove_piece(piece pc, int sq);

  inline static int index_from_pc(piece pc);

  /* private members */

  bitboard m_piece_boards[12];
  bitboard m_white_pieces;
  bitboard m_black_pieces;
  bitboard m_all_pieces;

  piece m_sq_board[64];

  bool m_white_turn;

  int m_white_king_loc;
  int m_black_king_loc;

  /* hashing items */
  uint64_t m_board_hash;
  uint64_t m_piece_hash;
  uint64_t m_pawn_hash;

  /* evaluation items */
  int m_material_score;
  int m_positional_score; // doesn't include kings
  int m_piece_counts[10];
  int m_total_material;

  std::vector<IrreversibleState> m_irr_state_history;

  /* for detecting repetition */
  /* I have to encode the ply of the last irreversible in the state */
  int m_ply;
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
