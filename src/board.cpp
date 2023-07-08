#include <string>
#include <stdlib.h>
#include <iostream>
#include <stack>

#include "include/board.h"
#include "include/bitboard.h"
#include "include/pieces.h"
#include "include/attacks.h"
#include "include/evaluation.h"
#include "include/hashing.h"
#include "include/tt.h"
#include "include/utils.h"

/// TODO: fix understanding of half-move and full-move clock 
/// TODO: full-move is currently unused 
Board::Board(std::string fen)
{
  std::string layout;
  std::string turn;
  std::string castling_rights;
  std::string en_passant_sq;
  int half_move_clock;
  int full_move_clock;

  int num_read = std::sscanf(fen.c_str(), "%s %s %s %s %d %d", layout, turn, castling_rights, en_passant_sq, &half_move_clock, &full_move_clock);
  if (num_read != 6)
  {
    std::cerr << "Invalid fen string!" << std::endl;
    exit(1);
  }

  int col = 0;
  int row = 7; 
  for (char c : layout)
  {
    if (isdigit(c))
    {
      col += c - '0';
      continue;
    }
    else if (c == '/')
    {
      row--;
      col = 0;
      continue;
    }
    
    // c is a piece 
    piece pc = utils::piece_from_fen_char(c);
    int pc_loc = row * 8 + col; 
    place_piece(pc, (Square)pc_loc);

    m_white_king_loc = (pc == WHITE | KING) ? (Square)pc_loc : m_white_king_loc;
    m_black_king_loc = (pc == BLACK | KING) ? (Square)pc_loc : m_black_king_loc;

    // evaluation stuff 
    if(PIECE(pc) != KING && PIECE(pc) != EMPTY) 
    {
      m_material_score += piece_values[index_from_pc(pc)];
      m_positional_score += piece_scores[index_from_pc(pc)][pc_loc];
      m_total_material += abs(piece_values[index_from_pc(pc)]);
    }
    col++;
  }

  update_redundant_boards(); // keep redundant info up to date 

  bool white_king_side{false};
  bool white_queen_side{false};
  bool black_king_side{false};
  bool black_queen_side{false};
  for (auto c : castling_rights)
  {
    switch (c)
    {
      case 'K':
        white_king_side = true;
        break;
      case 'Q':
        white_queen_side = true;
        break;
      case 'k':
        black_king_side = true;
        break;
      case 'q':
        black_queen_side = true;
        break;
      default:
        std::cerr << "Invalid fen string!" << std::endl;
        exit(1);
    };
  }

  Square en_passant = utils::sq_from_name(en_passant_sq);

  // add the state to the state history 
  IrreversibleState start_state{white_king_side,
                                white_queen_side,
                                black_king_side,
                                black_queen_side,
                                en_passant,
                                EMPTY,
                                half_move_clock,
                                0,
                                NO_MOVE};
  m_irr_state_history.push(start_state);

  m_board_hash = zobrist_hash(); /// TODO: these should all be a part of the board class 
  m_piece_hash = hash_pieces(); // hash the pieces initially
  m_pawn_hash = hash_pawns(); // hash the pawns initially
  m_board_hash_history.push_back(m_board_hash);

  /* more eval stuff */
  for(int i = 0; i < 10; i++) {
    m_piece_counts[i] = pop_count(m_piece_boards[i]);
  }
}

void Board::place_piece(piece pc, Square sq)
{
  if (sq == Square::NONE)
  {
    std::cerr << "Attempting to place piece on NONE square" << std::endl;
  }

  m_sq_board[(int)sq] = pc;
  place_piece_in_bb(pc, sq);
}

void Board::place_piece_in_bb(piece pc, Square sq)
{
  int index = index_from_pc(pc);
  m_piece_boards[index] |= (1 << (int)sq);
}

int Board::index_from_pc(piece pc)
{
  return pc - 2; /// TODO: make a comment on why this works
}

void Board::update_redundant_boards()
{
  m_white_pieces = (m_piece_boards[WHITE_PAWNS_INDEX]   | m_piece_boards[WHITE_KNIGHTS_INDEX] | 
                    m_piece_boards[WHITE_BISHOPS_INDEX] | m_piece_boards[WHITE_ROOKS_INDEX]   |
                    m_piece_boards[WHITE_QUEENS_INDEX]  | m_piece_boards[WHITE_KINGS_INDEX]);

  m_black_pieces = (m_piece_boards[BLACK_PAWNS_INDEX]   | m_piece_boards[BLACK_KNIGHTS_INDEX] | 
                    m_piece_boards[BLACK_BISHOPS_INDEX] | m_piece_boards[BLACK_ROOKS_INDEX]   |
                    m_piece_boards[BLACK_QUEENS_INDEX]  | m_piece_boards[BLACK_KINGS_INDEX]);

  m_all_pieces = m_white_pieces | m_black_pieces;
}

int Board::file(Square sq)
{
  return (int)sq & 7;
}

int Board::rank(Square sq)
{
  return (int)sq >> 3;
}

int Board::diag(Square sq)
{
  return 7 + rank(sq) - file(sq);
}

int Board::anti_diag(Square sq)
{
  return (int)sq >> 3;
}

/// TODO: this is actually appauling please fix this function (might need to rework how I do pieces)
Board::Pin Board::get_pinned_pieces(Square friendly_king_loc) const 
{
  Pin pin;
  pin.pinned_pieces = 0;
  bitboard curr_pin;
  Square pinned_piece_loc;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  bitboard friendly_pieces;
  if(m_white_turn) {
    opponent_rooks = m_piece_boards[BLACK_ROOKS_INDEX];
    opponent_bishops = m_piece_boards[BLACK_BISHOPS_INDEX];
    opponent_queens = m_piece_boards[BLACK_QUEENS_INDEX];
    friendly_pieces = m_white_pieces;
  }
  else {
    opponent_rooks = m_piece_boards[WHITE_ROOKS_INDEX];
    opponent_bishops = m_piece_boards[WHITE_BISHOPS_INDEX];
    opponent_queens = m_piece_boards[WHITE_QUEENS_INDEX];
    friendly_pieces = m_black_pieces;
  }
  bitboard king_rook_attacks = get_rook_attacks(friendly_king_loc, m_all_pieces);
  bitboard king_bishop_attacks = get_bishop_attacks(friendly_king_loc, m_all_pieces);
  bitboard rook_attacks;
  bitboard bishop_attacks;
  bitboard queen_attacks;
  Square pc_loc;
  int pc_rank;
  int pc_file;
  int pc_diag;
  int pc_antidiag;
  int king_rank = rank(friendly_king_loc);
  int king_file = file(friendly_king_loc);
  int king_diag = king_rank - king_file;
  int king_antidiag = king_rank + king_file;
  while(opponent_rooks) {
    pc_loc = (Square)first_set_bit(opponent_rooks);
    pc_rank = rank(pc_loc);
    pc_file = file(pc_loc);
    if(!(pc_rank == king_rank || pc_file == king_file)) {
      REMOVE_FIRST(opponent_rooks);
      continue;
    }
    rook_attacks = get_rook_attacks(pc_loc, m_all_pieces);
    curr_pin = rook_attacks & king_rook_attacks & friendly_pieces;
    if(curr_pin){
      pin.pinned_pieces |= curr_pin;
      pinned_piece_loc = (Square)first_set_bit(curr_pin);
      pin.ray_at_sq[(int)pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
    }
    REMOVE_FIRST(opponent_rooks);
  }
  while(opponent_bishops) {
    pc_loc = (Square)first_set_bit(opponent_bishops);
    pc_rank = rank(pc_loc);
    pc_file = file(pc_loc);
    pc_diag = pc_rank - pc_file;
    pc_antidiag = pc_rank + pc_file;
    if(!(pc_diag == king_diag || pc_antidiag == king_antidiag)) {
      REMOVE_FIRST(opponent_bishops);
      continue;
    }
    bishop_attacks = get_bishop_attacks(pc_loc, m_all_pieces);
    curr_pin = bishop_attacks & king_bishop_attacks & friendly_pieces;
    if(curr_pin){
      pin.pinned_pieces |= curr_pin;
      pinned_piece_loc = (Square)first_set_bit(curr_pin);
      pin.ray_at_sq[(int)pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
    }
    REMOVE_FIRST(opponent_bishops);
  }
  while(opponent_queens) {
    pc_loc = (Square)first_set_bit(opponent_queens);
    pc_rank = rank(pc_loc);
    pc_file = file(pc_loc);
    pc_diag = pc_rank - pc_file;
    pc_antidiag = pc_rank + pc_file;
    if(pc_rank == king_rank || pc_file == king_file) {
      queen_attacks = get_rook_attacks(pc_loc, m_all_pieces);
      curr_pin = queen_attacks & king_rook_attacks & friendly_pieces;
      if(curr_pin){
        pin.pinned_pieces |= curr_pin;
        pinned_piece_loc = (Square)first_set_bit(curr_pin);
        pin.ray_at_sq[(int)pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
      }
    }
    else if(pc_diag == king_diag || pc_antidiag == king_antidiag){
      queen_attacks = get_bishop_attacks(pc_loc, m_all_pieces);
      curr_pin = queen_attacks & king_bishop_attacks & friendly_pieces;
      if(curr_pin){
        pin.pinned_pieces |= curr_pin;
        pinned_piece_loc = (Square)first_set_bit(curr_pin);
        pin.ray_at_sq[(int)pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
      }
    }
    REMOVE_FIRST(opponent_queens);
  }
  return pin;
}

bitboard Board::checking_pieces() const 
{
  Square friendly_king = (m_white_turn) ? m_white_king_loc : m_black_king_loc;
  move_t last_move = m_irr_state_history.top().get_last_move();
  if(last_move == NO_MOVE) {
    return attackers_from_square(friendly_king);
  }
  bool discovered_check = false;

  int to = TO(last_move);
  int from = FROM(last_move);
  int flags = FLAGS(last_move);

  bitboard checkers = 0;

  piece moved_piece = m_sq_board[to];
  bitboard piece_bit = BIT_FROM_SQ(to);
  switch (PIECE(moved_piece)) {
    case PAWN:
      checkers = get_pawn_attacks(friendly_king, m_white_turn) & piece_bit;
      break;
    case KNIGHT:
      checkers = get_knight_attacks(friendly_king) & piece_bit;
      break;
    case BISHOP:
      checkers = get_bishop_attacks(friendly_king, m_all_pieces) & piece_bit;
      break;
    case ROOK:
      checkers = get_rook_attacks(friendly_king, m_all_pieces) & piece_bit;
      break;
    case QUEEN:
      checkers = get_queen_attacks(friendly_king, m_all_pieces) & piece_bit;
      break;
    case KING:
      break; /* the king can never move to check the other king */
  }

  /* there is no possibility for a discovered check here */
  if(flags == QUIET_MOVE || flags == DOUBLE_PUSH || flags == NORMAL_CAPTURE) {
    if(rank((Square)from) != rank((Square)friendly_king) && file((Square)from) != file((Square)friendly_king) &&
       diag((Square)from) != diag((Square)friendly_king) && anti_diag((Square)from) != anti_diag((Square)friendly_king))
       return checkers;
  }


  bitboard opponent_bishops;
  bitboard opponent_rooks;
  bitboard opponent_queens;

  if(m_white_turn) {
    opponent_bishops = m_piece_boards[BLACK_BISHOPS_INDEX];
    opponent_rooks = m_piece_boards[BLACK_ROOKS_INDEX];
    opponent_queens = m_piece_boards[BLACK_QUEENS_INDEX];
  }
  else {
    opponent_bishops = m_piece_boards[WHITE_BISHOPS_INDEX];
    opponent_rooks = m_piece_boards[WHITE_ROOKS_INDEX];
    opponent_queens = m_piece_boards[WHITE_QUEENS_INDEX];
  }
  bitboard diagonal_sliders = opponent_bishops | opponent_queens;
  bitboard cardinal_sliders = opponent_rooks | opponent_queens;

  /* remove the piece that was moved to not double check it */
  diagonal_sliders &= ~BIT_FROM_SQ(to);
  cardinal_sliders &= ~BIT_FROM_SQ(to);

  bitboard bishop_from_king = get_bishop_attacks(friendly_king, m_all_pieces);
  bitboard attacking_diagonal = bishop_from_king & diagonal_sliders;
  checkers |= attacking_diagonal;
  if(attacking_diagonal)
    discovered_check = true;

  if(!discovered_check){
    bitboard rook_from_king = get_rook_attacks(friendly_king, m_all_pieces);
    checkers |= rook_from_king & cardinal_sliders;
  }
  return checkers;
}

Board::CheckType Board::check_type(bitboard checkers) const
{
  if(!checkers) return CheckType::NONE;
  REMOVE_FIRST(checkers);
  if(!checkers) return CheckType::SINGLE;
  return CheckType::DOUBLE;
}

bool Board::in_check() const
{
  return check_type(checking_pieces()) != CheckType::NONE;
}

bool Board::is_repetition() const 
{
  int irr_ply = m_irr_state_history.top().get_irr_ply();
  /* we start searching at the previous board state, and up to and including the board with 
     the most recent irreversible move played on the board. We decrement by two since we only need to check
     board states where it was the current player's turn. */
  for(int i = m_ply - 2; i >= irr_ply; i = i - 2) 
  {
    if(m_board_hash_history[i] == m_board_hash)
    {
      return true;
    }
  }
  return false;
}

Board::IrreversibleState::IrreversibleState(bool white_ks, 
                                            bool white_qs, 
                                            bool black_ks, 
                                            bool black_qs, 
                                            Square en_passant_sq, 
                                            piece last_capture, 
                                            uint16_t fifty_move_count,
                                            uint32_t irr_ply,
                                            move_t last_move)
{
  set_white_king_side_castle(white_ks);
  set_white_queen_side_castle(white_qs);
  set_black_king_side_castle(black_ks);
  set_black_queen_side_castle(black_qs);
  set_en_passant_sq(en_passant_sq);
  set_last_capture(last_capture);
  set_fifty_move(fifty_move_count);
  set_irr_ply(irr_ply);
  set_last_move(last_move);
}