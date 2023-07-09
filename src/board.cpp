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

///////////////////////////////////////////////// BOARD CREATION /////////////////////////////////////////////////

Board::Board()
{
  reset();
}

Board::Board(std::string fen)
{
  reset(fen);
}

void Board::reset()
{
  reset(constants::STARTFEN);
}

void Board::reset(std::string fen)
{
  clear(); // clear the board
  // decode the fen string 
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
    place_piece(pc, pc_loc);

    m_white_king_loc = (pc == WHITE | KING) ? pc_loc : m_white_king_loc;
    m_black_king_loc = (pc == BLACK | KING) ? pc_loc : m_black_king_loc;

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

  int en_passant = utils::sq_from_name(en_passant_sq);

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
  m_irr_state_history.clear();
  m_irr_state_history.push_back(start_state);

  m_board_hash = zobrist_hash(); /// TODO: these should all be a part of the board class 
  m_piece_hash = hash_pieces(); // hash the pieces initially
  m_pawn_hash = hash_pawns(); // hash the pawns initially
  m_board_hash_history.clear();
  m_board_hash_history.push_back(m_board_hash);

  /* more eval stuff */
  for(int i = 0; i < 10; i++) {
    m_piece_counts[i] = pop_count(m_piece_boards[i]);
  }
}

void Board::clear()
{
  // clear out all of the bitboards
  for (size_t i = 0; i < constants::NUM_PIECE_TYPES; i++)
  {
    m_piece_boards[i] = 0;
  }
  m_white_pieces = 0;
  m_black_pieces = 0;
  m_all_pieces = 0;

  for (size_t i = 0; i < 64; i++)
  {
    m_sq_board[i] = EMPTY;
  }

  m_white_king_loc = NONE;
  m_black_king_loc = NONE;

  m_board_hash = 0;
  m_piece_hash = 0;
  m_pawn_hash = 0;

  m_material_score = 0;
  m_positional_score = 0;
  m_total_material = 0;

  for (size_t i = 0; i < constants::NUM_PIECE_TYPES - 2; i++) // don't include kings here
  {
    m_piece_counts[i] = 0;
  }

  m_irr_state_history.clear();
  m_ply = 0;
  m_board_hash_history.clear();
}

///////////////////////////////////////////////// BOARD MANIPULATION /////////////////////////////////////////////////

void Board::make_move(move_t move) 
{
  /* make a copy of the irreversible aspects of the position */
  IrreversibleState prev_state = m_irr_state_history.back();
  IrreversibleState state = prev_state; // make a copy 
  m_ply++; /* we do this here to be able to update irr_ply */

  hash_val board_hash = m_board_hash;
  hash_val piece_hash = m_piece_hash;
  hash_val pawn_hash = m_pawn_hash;

  int from = FROM(move);
  int to = TO(move);
  int flags = FLAGS(move);

  /* 
    always have to remove the piece from its square...
    if promotion, you cannot place the same piece on to square
   */
  piece moving_piece = m_sq_board[from];
  remove_piece(moving_piece, from);

  if(PIECE(moving_piece) != KING) // king done seperately during eval for endgame
    m_positional_score -= piece_scores[index_from_pc(moving_piece)][from];
  
  /* XOR out the piece from hash value */
  hash_val from_zobrist = zobrist_table.table[from][index_from_pc(moving_piece)];
  board_hash ^= from_zobrist;
  piece_hash ^= from_zobrist;
  if(PIECE(moving_piece) == PAWN) {
    pawn_hash ^= from_zobrist;
  }
  
  /* update the king locations and castling rights */
  if(moving_piece == (WHITE | KING)) {
    m_white_king_loc = to;
    state.set_white_castle(false);
  }
  else if(moving_piece == (BLACK | KING)) {
    m_black_king_loc = to;
    state.set_black_castle(false);
  }
  else if(moving_piece == (WHITE | ROOK) && from == H1) {
    state.set_white_king_side_castle(false);
  }
  else if(moving_piece == (WHITE | ROOK) && from == A1) {
    state.set_white_queen_side_castle(false);
  }
  else if(moving_piece == (BLACK | ROOK) && from == H8) {
    state.set_black_king_side_castle(false);
  }
  else if(moving_piece == (BLACK | ROOK) && from == A8) {
    state.set_black_queen_side_castle(false);
  }

  /* default there to be no en passant Square and set it if double pawn push */
  state.set_en_passant_sq(NONE);

  bitboard *rook_board;
  piece captured_piece = EMPTY;
  bitboard *captured_board;
  piece promo_piece = EMPTY;
  bitboard *promo_board;
  int opponent_pawn_sq;
  hash_val to_zobrist = zobrist_table.table[to][index_from_pc(moving_piece)];
  switch (flags) {
    case QUIET_MOVE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place moving piece in hash value
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) 
        pawn_hash ^= to_zobrist;
      break;
    case DOUBLE_PUSH:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the pawn in hash value
      piece_hash ^= to_zobrist;
      pawn_hash ^= to_zobrist;
      /* 
        if it's a double pawn push and we are starting on rank 2, its white pushing
        otherwise it is black pushing the pawn
      */
      if(rank(from) == Rank::RANK_2)
        state.set_en_passant_sq(from + 8);
      else
        state.set_en_passant_sq(from + 8);
      // hash value update for en passant Square happens later
      break;
    case KING_SIDE_CASTLE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the king in hash value
      piece_hash ^= to_zobrist;
      if(from == E1) { // white king side
        remove_piece(WHITE | ROOK, H1);
        place_piece(WHITE | ROOK, F1);
        board_hash ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // remove white rook from H1
        board_hash ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // place white rook on F1
        piece_hash ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // remove white rook from H1
        piece_hash ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // place white rook on F1
        m_positional_score -= piece_scores[WHITE_ROOKS_INDEX][H1];
        m_positional_score += piece_scores[WHITE_ROOKS_INDEX][F1];
      }
      else { // black king side
        remove_piece(BLACK | ROOK, H8);
        place_piece(BLACK | ROOK, F8);
        board_hash ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // remove black rook from H8
        board_hash ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // place black rook on F8
        piece_hash ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // remove black rook from H8
        piece_hash ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // place black rook on F8
        m_positional_score -= piece_scores[BLACK_ROOKS_INDEX][H8];
        m_positional_score += piece_scores[BLACK_ROOKS_INDEX][F8];
      }
      break;
    case QUEEN_SIDE_CASTLE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the king in hash value
      // piece_hash ^= to_zobrist;
      if(from == E1) { // white queen side
        remove_piece(WHITE | ROOK, A1);
        place_piece(WHITE | ROOK, D1);
        board_hash ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // remove white rook from A1
        board_hash ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // place white rook on D1
        piece_hash ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // remove white rook from A1
        piece_hash ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // place white rook on D1
        m_positional_score -= piece_scores[WHITE_ROOKS_INDEX][A1];
        m_positional_score += piece_scores[WHITE_ROOKS_INDEX][D1];
      }
      else { // black queen side
        remove_piece(BLACK | ROOK, A8);
        place_piece(BLACK | ROOK, D8);
        board_hash ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // remove black rook from A8
        board_hash ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // place black rook on D8
        piece_hash ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // remove black rook from A8
        piece_hash ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // place black rook on D8
        m_positional_score -= piece_scores[BLACK_ROOKS_INDEX][A8];
        m_positional_score += piece_scores[BLACK_ROOKS_INDEX][D8];
      }
      break;
    case NORMAL_CAPTURE:
      place_piece_in_bb(moving_piece, to); 
      board_hash ^= to_zobrist; // place the moving piece in hash value
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) 
        pawn_hash ^= to_zobrist;

      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);

      m_sq_board[to] = moving_piece;

      board_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)];
      if(PIECE(captured_piece) == PAWN)
        pawn_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)];

      /* remove castling rights if rook is captured in corner */
      if(PIECE(captured_piece) == ROOK) {
        if      (to == H1) state.set_white_king_side_castle(false);
        else if (to == A1) state.set_white_queen_side_castle(false);
        else if (to == H8) state.set_black_king_side_castle(false);
        else if (to == A8) state.set_black_queen_side_castle(false);
      }
      break;
    case EN_PASSANT_CAPTURE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the pawn in hash value
      piece_hash ^= to_zobrist;
      pawn_hash ^= to_zobrist;

      /* distinguish between white and black en passant */
      opponent_pawn_sq = (rank(to) == Rank::RANK_6) ? (to - 8) : (to + 8);

      /* remove the captured pawn */
      captured_piece = m_sq_board[opponent_pawn_sq];
      remove_piece(captured_piece, opponent_pawn_sq);
      board_hash ^= zobrist_table.table[opponent_pawn_sq][index_from_pc(captured_piece)]; // remove the captured pawn from hash value
      piece_hash ^= zobrist_table.table[opponent_pawn_sq][index_from_pc(captured_piece)];
      pawn_hash ^= zobrist_table.table[opponent_pawn_sq][index_from_pc(captured_piece)];
      break;
    case KNIGHT_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)];
      /* fallthrough */
    case KNIGHT_PROMO:
      if(m_white_turn) {promo_piece = WHITE | KNIGHT;}
      else             {promo_piece = BLACK | KNIGHT;}
      place_piece(promo_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)]; // place knight in hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)];
      break;
    case BISHOP_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)];
      /* fallthrough */
    case BISHOP_PROMO:
      if(m_white_turn) {promo_piece = WHITE | BISHOP;}
      else             {promo_piece = BLACK | BISHOP;}
      place_piece(promo_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)]; // place bishop in hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)];
      break;
    case ROOK_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)];
      /* fallthrough */
    case ROOK_PROMO:
      if(m_white_turn) {promo_piece = WHITE | ROOK;}
      else             {promo_piece = BLACK | ROOK;}
      place_piece(promo_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)]; // place rook in hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)];
      break;
    case QUEEN_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(captured_piece)];
      /* fallthrough */
    case QUEEN_PROMO:
      if(m_white_turn) {promo_piece = WHITE | QUEEN;}
      else             {promo_piece = BLACK | QUEEN;}
      place_piece(promo_piece, to);
      board_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)]; // place queen in hash value
      piece_hash ^= zobrist_table.table[to][index_from_pc(promo_piece)];
      break;
  }

  /* update hash value castling rights */
  if(m_white_turn) { // castling rights have changed
    if(prev_state.can_white_king_side_castle() && !state.can_white_king_side_castle())
      board_hash ^= zobrist_table.white_king_side;
    if(prev_state.can_white_queen_side_castle() && !state.can_white_queen_side_castle())
      board_hash ^= zobrist_table.white_queen_side;
  }
  else { 
    if(prev_state.can_black_king_side_castle() && !state.can_black_king_side_castle())
      board_hash ^= zobrist_table.black_king_side;
    if(prev_state.can_black_queen_side_castle() && !state.can_black_queen_side_castle())
      board_hash ^= zobrist_table.black_queen_side;
  }

  /* update en passant file in hash value */
  int prev_en_passant_sq = prev_state.get_en_passant_sq();
  int curr_en_passant_sq = state.get_en_passant_sq();
  if(prev_en_passant_sq != NONE)
    board_hash ^= zobrist_table.en_passant_file[file(prev_en_passant_sq)]; // remove the last board's en passant from hash value
  
  if(curr_en_passant_sq != NONE)
    board_hash ^= zobrist_table.en_passant_file[file(curr_en_passant_sq)]; // place the current en passant file in hash value

  /* add the last captured piece to the state */
  state.set_last_capture(captured_piece);

  /* update evaluation items */
  if(captured_piece != EMPTY){
    if(flags == EN_PASSANT_CAPTURE)
      m_positional_score -= piece_scores[index_from_pc(captured_piece)][opponent_pawn_sq];
    else
      m_positional_score -= piece_scores[index_from_pc(captured_piece)][to];
    m_material_score -= piece_values[index_from_pc(captured_piece)];
    m_piece_counts[index_from_pc(captured_piece)]--;
    m_total_material -= abs(piece_values[index_from_pc(captured_piece)]);
  }

  if(promo_piece != EMPTY) {
    if(COLOR(promo_piece) == WHITE) {
      m_material_score -= piece_values[WHITE_PAWNS_INDEX];
      m_piece_counts[WHITE_PAWNS_INDEX]--;
      m_total_material -= abs(piece_values[WHITE_PAWNS_INDEX]);
    }
    else {
      m_material_score -= piece_values[BLACK_PAWNS_INDEX];
      m_piece_counts[BLACK_PAWNS_INDEX]--;
      m_total_material -= abs(piece_values[BLACK_PAWNS_INDEX]);
    }
    m_material_score += piece_values[index_from_pc(promo_piece)];
    m_positional_score += piece_scores[index_from_pc(promo_piece)][to];
    m_piece_counts[index_from_pc(promo_piece)]++;
    m_total_material += abs(piece_values[index_from_pc(promo_piece)]);
  }
  else if(PIECE(moving_piece) != KING) {
    m_positional_score += piece_scores[index_from_pc(moving_piece)][to];
  }

  /* if we make an irreversible move, remember it! */
  if(IS_CAPTURE(move) || IS_PROMO(move) || PIECE(moving_piece) == PAWN)
    state.set_irr_ply(m_ply);
  
  m_white_turn = !m_white_turn;
  /* reverse the black_to_move hash */
  board_hash ^= zobrist_table.black_to_move;

  update_redundant_boards();
  state.set_last_move(move);
  m_board_hash = board_hash;
  m_piece_hash = piece_hash;
  m_pawn_hash = pawn_hash;
  m_irr_state_history.push_back(state);
  m_board_hash_history.push_back(board_hash);
}

void Board::place_piece(piece pc, int sq)
{
  if (sq == NONE)
  {
    std::cerr << "Attempting to place piece on NONE square" << std::endl;
  }

  m_sq_board[sq] = pc;
  place_piece_in_bb(pc, sq);
}

void Board::place_piece_in_bb(piece pc, int sq)
{
  int index = index_from_pc(pc);
  m_piece_boards[index] |= (1 << sq);
}

void Board::remove_piece(piece pc, int sq)
{
  if (sq == NONE)
  {
    std::cerr << "Attempting to remove piece from NONE square" << std::endl;
  }

  m_sq_board[sq] = EMPTY;
  remove_piece_from_bb(pc, sq);
}

void Board::remove_piece_from_bb(piece pc, int sq)
{
  int index = index_from_pc(pc);
  m_piece_boards[index] &= ~(1 << sq);
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

/// TODO: this is actually appauling please fix this function (might need to rework how I do pins)
Board::Pin Board::get_pinned_pieces(int friendly_king_loc) const 
{
  Pin pin;
  pin.pinned_pieces = 0;
  bitboard curr_pin;
  int pinned_piece_loc;
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
  int pc_loc;
  int pc_rank;
  int pc_file;
  int pc_diag;
  int pc_antidiag;
  int king_rank = rank(friendly_king_loc);
  int king_file = file(friendly_king_loc);
  int king_diag = king_rank - king_file;
  int king_antidiag = king_rank + king_file;
  while(opponent_rooks) {
    pc_loc = first_set_bit(opponent_rooks);
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
      pinned_piece_loc = first_set_bit(curr_pin);
      pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
    }
    REMOVE_FIRST(opponent_rooks);
  }
  while(opponent_bishops) {
    pc_loc = first_set_bit(opponent_bishops);
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
      pinned_piece_loc = first_set_bit(curr_pin);
      pin.ray_at_sq[pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
    }
    REMOVE_FIRST(opponent_bishops);
  }
  while(opponent_queens) {
    pc_loc = first_set_bit(opponent_queens);
    pc_rank = rank(pc_loc);
    pc_file = file(pc_loc);
    pc_diag = pc_rank - pc_file;
    pc_antidiag = pc_rank + pc_file;
    if(pc_rank == king_rank || pc_file == king_file) {
      queen_attacks = get_rook_attacks(pc_loc, m_all_pieces);
      curr_pin = queen_attacks & king_rook_attacks & friendly_pieces;
      if(curr_pin){
        pin.pinned_pieces |= curr_pin;
        pinned_piece_loc = first_set_bit(curr_pin);
        pin.ray_at_sq[pinned_piece_loc] = get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
      }
    }
    else if(pc_diag == king_diag || pc_antidiag == king_antidiag){
      queen_attacks = get_bishop_attacks(pc_loc, m_all_pieces);
      curr_pin = queen_attacks & king_bishop_attacks & friendly_pieces;
      if(curr_pin){
        pin.pinned_pieces |= curr_pin;
        pinned_piece_loc = first_set_bit(curr_pin);
        pin.ray_at_sq[pinned_piece_loc] = get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
      }
    }
    REMOVE_FIRST(opponent_queens);
  }
  return pin;
}

bitboard Board::checking_pieces() const 
{
  int friendly_king = (m_white_turn) ? m_white_king_loc : m_black_king_loc;
  move_t last_move = m_irr_state_history.back().get_last_move();
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
    if(rank(from) != rank(friendly_king) && file(from) != file(friendly_king) &&
       diag(from) != diag(friendly_king) && anti_diag(from) != anti_diag(friendly_king))
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
  if(!checkers) return CheckType::NO_CHECK;
  REMOVE_FIRST(checkers);
  if(!checkers) return CheckType::SINGLE;
  return CheckType::DOUBLE;
}

bool Board::in_check() const
{
  return check_type(checking_pieces()) != CheckType::NO_CHECK;
}

bool Board::is_repetition() const 
{
  int irr_ply = m_irr_state_history.back().get_irr_ply();
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
                                            int en_passant_sq, 
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


///////////////////////////////////////////////// HELPFUL BOARD FUNCTIONS /////////////////////////////////////////////////

int Board::file(int sq)
{
  return sq & 7;
}

int Board::rank(int sq)
{
  return sq >> 3;
}

int Board::diag(int sq)
{
  return 7 + rank(sq) - file(sq);
}

int Board::anti_diag(int sq)
{
  return rank(sq) + file(sq);
}