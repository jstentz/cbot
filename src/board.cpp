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
  char layout_buf[100];
  char turn_buf[100];
  char castling_rights_buf[100];
  char en_passant_sq_buf[100];
  uint16_t half_move_clock;
  uint16_t full_move_clock;

  int num_read = std::sscanf(fen.c_str(), "%s %s %s %s %d %d", layout_buf, turn_buf, castling_rights_buf, en_passant_sq_buf, &half_move_clock, &full_move_clock);
  if (num_read != 6)
  {
    std::cerr << "Invalid fen string!" << std::endl;
    exit(1);
  }
  /// TODO: fix this
  std::string layout{layout_buf};
  std::string turn{turn_buf};
  std::string castling_rights{castling_rights_buf};
  std::string en_passant_sq{en_passant_sq_buf};

  m_white_turn = turn == "w";

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
      m_material_score += constants::piece_values[utils::index_from_pc(pc)];
      m_positional_score += constants::piece_scores[utils::index_from_pc(pc)][pc_loc];
      m_total_material += abs(constants::piece_values[utils::index_from_pc(pc)]);
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
                                Move::NO_MOVE};
  m_irr_state_history.clear();
  m_irr_state_history.push_back(start_state);

  m_board_hash = m_hasher.hash_board(m_white_turn, m_sq_board, white_king_side, white_queen_side, black_king_side, black_queen_side, en_passant); 
  m_piece_hash = m_hasher.hash_pieces(m_sq_board); 
  m_pawn_hash = m_hasher.hash_pawns(m_sq_board); 
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

  m_white_king_loc = constants::NONE;
  m_black_king_loc = constants::NONE;

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

void Board::make_move(Move move) 
{
  if (move.is_no_move())
  {
    std::cerr << "Attempting to make no_move!\n";
    return;
  }
  /* make a copy of the irreversible aspects of the position */
  IrreversibleState prev_state = m_irr_state_history.back();
  IrreversibleState state = prev_state; // make a copy 
  m_ply++; /* we do this here to be able to update irr_ply */

  uint64_t board_hash = m_board_hash;
  uint64_t piece_hash = m_piece_hash;
  uint64_t pawn_hash = m_pawn_hash;

  int from = move.from();
  int to = move.to();
  int type = move.type();

  /* 
    always have to remove the piece from its square...
    if promotion, you cannot place the same piece on to square
   */
  piece moving_piece = m_sq_board[from];
  remove_piece(moving_piece, from);

  if(PIECE(moving_piece) != KING) // king done seperately during eval for endgame
    m_positional_score -= constants::piece_scores[utils::index_from_pc(moving_piece)][from];
  
  /* XOR out the piece from hash value */
  uint64_t from_zobrist = m_hasher.get_hash_val(moving_piece, from);
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
  else if(moving_piece == (WHITE | ROOK) && from == constants::H1) {
    state.set_white_king_side_castle(false);
  }
  else if(moving_piece == (WHITE | ROOK) && from == constants::A1) {
    state.set_white_queen_side_castle(false);
  }
  else if(moving_piece == (BLACK | ROOK) && from == constants::H8) {
    state.set_black_king_side_castle(false);
  }
  else if(moving_piece == (BLACK | ROOK) && from == constants::A8) {
    state.set_black_queen_side_castle(false);
  }

  /* default there to be no en passant Square and set it if double pawn push */
  state.set_en_passant_sq(constants::NONE);

  bitboard *rook_board;
  piece captured_piece = EMPTY;
  bitboard *captured_board;
  piece promo_piece = EMPTY;
  bitboard *promo_board;
  int opponent_pawn_sq;
  uint64_t to_zobrist = m_hasher.get_hash_val(moving_piece, to);
  switch (type) {
    case Move::QUIET_MOVE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place moving piece in hash value
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) 
        pawn_hash ^= to_zobrist;
      break;
    case Move::DOUBLE_PUSH:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the pawn in hash value
      piece_hash ^= to_zobrist;
      pawn_hash ^= to_zobrist;
      /* 
        if it's a double pawn push and we are starting on rank 2, its white pushing
        otherwise it is black pushing the pawn
      */
      if(utils::rank(from) == constants::RANK_2)
        state.set_en_passant_sq(from + 8);
      else
        state.set_en_passant_sq(from + 8);
      // hash value update for en passant Square happens later
      break;
    case Move::KING_SIDE_CASTLE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the king in hash value
      piece_hash ^= to_zobrist;
      if(from == constants::E1) { // white king side
        remove_piece(WHITE | ROOK, constants::H1);
        place_piece(WHITE | ROOK, constants::F1);
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::H1); // remove white rook from H1
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::F1); // place white rook on F1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::H1); // remove white rook from H1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::F1); // place white rook on F1
        m_positional_score -= constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::H1];
        m_positional_score += constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::F1];
      }
      else { // black king side
        remove_piece(BLACK | ROOK, constants::H8);
        place_piece(BLACK | ROOK, constants::F8);
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::H8); // remove black rook from H8
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::F8); // place black rook on F8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::H8); // remove black rook from H8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::F8); // place black rook on F8
        m_positional_score -= constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::H8];
        m_positional_score += constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::F8];
      }
      break;
    case Move::QUEEN_SIDE_CASTLE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the king in hash value
      // piece_hash ^= to_zobrist;
      if(from == constants::E1) { // white queen side
        remove_piece(WHITE | ROOK, constants::A1);
        place_piece(WHITE | ROOK, constants::D1);
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::A1); // remove white rook from A1
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::D1); // place white rook on D1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::A1); // remove white rook from A1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::D1); // place white rook on D1
        m_positional_score -= constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::A1];
        m_positional_score += constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::D1];
      }
      else { // black queen side
        remove_piece(BLACK | ROOK, constants::A8);
        place_piece(BLACK | ROOK, constants::D8);
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::A8); // remove black rook from A8
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::D8); // place black rook on D8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::A8); // remove black rook from A8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::D8); // place black rook on D8
        m_positional_score -= constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::A8];
        m_positional_score += constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::D8];
      }
      break;
    case Move::NORMAL_CAPTURE:
      place_piece_in_bb(moving_piece, to); 
      board_hash ^= to_zobrist; // place the moving piece in hash value
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) 
        pawn_hash ^= to_zobrist;

      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);

      m_sq_board[to] = moving_piece;

      board_hash ^= m_hasher.get_hash_val(captured_piece, to); // remove the captured piece from hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      if(PIECE(captured_piece) == PAWN)
        pawn_hash ^= m_hasher.get_hash_val(captured_piece, to);

      /* remove castling rights if rook is captured in corner */
      if(PIECE(captured_piece) == ROOK) {
        if      (to == constants::H1) state.set_white_king_side_castle(false);
        else if (to == constants::A1) state.set_white_queen_side_castle(false);
        else if (to == constants::H8) state.set_black_king_side_castle(false);
        else if (to == constants::A8) state.set_black_queen_side_castle(false);
      }
      break;
    case Move::EN_PASSANT_CAPTURE:
      place_piece(moving_piece, to);
      board_hash ^= to_zobrist; // place the pawn in hash value
      piece_hash ^= to_zobrist;
      pawn_hash ^= to_zobrist;

      /* distinguish between white and black en passant */
      opponent_pawn_sq = (utils::rank(to) == constants::RANK_6) ? (to - 8) : (to + 8);

      /* remove the captured pawn */
      captured_piece = m_sq_board[opponent_pawn_sq];
      remove_piece(captured_piece, opponent_pawn_sq);
      board_hash ^= m_hasher.get_hash_val(captured_piece, opponent_pawn_sq); // remove the captured pawn from hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, opponent_pawn_sq);
      pawn_hash ^= m_hasher.get_hash_val(captured_piece, opponent_pawn_sq);
      break;
    case Move::KNIGHT_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); // remove the captured piece from hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::KNIGHT_PROMO:
      if(m_white_turn) {promo_piece = WHITE | KNIGHT;}
      else             {promo_piece = BLACK | KNIGHT;}
      place_piece(promo_piece, to);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to); // place knight in hash value
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      break;
    case Move::BISHOP_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); // remove the captured piece from hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::BISHOP_PROMO:
      if(m_white_turn) {promo_piece = WHITE | BISHOP;}
      else             {promo_piece = BLACK | BISHOP;}
      place_piece(promo_piece, to);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to); // place bishop in hash value
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      break;
    case Move::ROOK_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); // remove the captured piece from hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::ROOK_PROMO:
      if(m_white_turn) {promo_piece = WHITE | ROOK;}
      else             {promo_piece = BLACK | ROOK;}
      place_piece(promo_piece, to);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to); // place rook in hash value
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      break;
    case Move::QUEEN_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = m_sq_board[to];
      remove_piece_from_bb(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); // remove the captured piece from hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::QUEEN_PROMO:
      if(m_white_turn) {promo_piece = WHITE | QUEEN;}
      else             {promo_piece = BLACK | QUEEN;}
      place_piece(promo_piece, to);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to); // place queen in hash value
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      break;
  }

  /* update hash value castling rights */
  if(m_white_turn) { // castling rights have changed
    if(prev_state.can_white_king_side_castle() && !state.can_white_king_side_castle())
      board_hash ^= m_hasher.get_white_king_side_hash();
    if(prev_state.can_white_queen_side_castle() && !state.can_white_queen_side_castle())
      board_hash ^= m_hasher.get_white_queen_side_hash();
  }
  else { 
    if(prev_state.can_black_king_side_castle() && !state.can_black_king_side_castle())
      board_hash ^= m_hasher.get_black_king_side_hash();
    if(prev_state.can_black_queen_side_castle() && !state.can_black_queen_side_castle())
      board_hash ^= m_hasher.get_black_queen_side_hash();
  }

  /* update en passant file in hash value */
  int prev_en_passant_sq = prev_state.get_en_passant_sq();
  int curr_en_passant_sq = state.get_en_passant_sq();
  if(prev_en_passant_sq != constants::NONE)
    board_hash ^= m_hasher.get_en_passant_hash(prev_en_passant_sq); // remove the last board's en passant from hash value
  
  if(curr_en_passant_sq != constants::NONE)
    board_hash ^= m_hasher.get_en_passant_hash(curr_en_passant_sq); // place the current en passant file in hash value

  /* add the last captured piece to the state */
  state.set_last_capture(captured_piece);

  /* update evaluation items */
  if(captured_piece != EMPTY){
    if(type == Move::EN_PASSANT_CAPTURE)
      m_positional_score -= constants::piece_scores[utils::index_from_pc(captured_piece)][opponent_pawn_sq];
    else
      m_positional_score -= constants::piece_scores[utils::index_from_pc(captured_piece)][to];
    m_material_score -= constants::piece_values[utils::index_from_pc(captured_piece)];
    m_piece_counts[utils::index_from_pc(captured_piece)]--;
    m_total_material -= abs(constants::piece_values[utils::index_from_pc(captured_piece)]);
  }

  if(promo_piece != EMPTY) {
    if(COLOR(promo_piece) == WHITE) {
      m_material_score -= constants::piece_values[constants::WHITE_PAWNS_INDEX];
      m_piece_counts[constants::WHITE_PAWNS_INDEX]--;
      m_total_material -= abs(constants::piece_values[constants::WHITE_PAWNS_INDEX]);
    }
    else {
      m_material_score -= constants::piece_values[constants::BLACK_PAWNS_INDEX];
      m_piece_counts[constants::BLACK_PAWNS_INDEX]--;
      m_total_material -= abs(constants::piece_values[constants::BLACK_PAWNS_INDEX]);
    }
    m_material_score += constants::piece_values[utils::index_from_pc(promo_piece)];
    m_positional_score += constants::piece_scores[utils::index_from_pc(promo_piece)][to];
    m_piece_counts[utils::index_from_pc(promo_piece)]++;
    m_total_material += abs(constants::piece_values[utils::index_from_pc(promo_piece)]);
  }
  else if(PIECE(moving_piece) != KING) {
    m_positional_score += constants::piece_scores[utils::index_from_pc(moving_piece)][to];
  }

  /* if we make an irreversible move, remember it! */
  if(move.is_capture() || move.is_promo() || PIECE(moving_piece) == PAWN)
    state.set_irr_ply(m_ply);
  
  m_white_turn = !m_white_turn;
  /* reverse the black_to_move hash */
  board_hash ^= m_hasher.get_black_to_move_hash();

  update_redundant_boards();
  state.set_last_move(move);
  m_board_hash = board_hash;
  m_piece_hash = piece_hash;
  m_pawn_hash = pawn_hash;
  m_irr_state_history.push_back(state);
  m_board_hash_history.push_back(board_hash);
}

void Board::unmake_move(Move move) {
  if (move.is_no_move())
  {
    std::cerr << "Attempting to unmake no_move!\n";
    return;
  }
  /* make a copy of the irreversible aspects of the position */
  IrreversibleState state = m_irr_state_history.back();
  m_ply--;
  m_board_hash_history.pop_back();

  uint64_t board_hash = m_board_hash;
  uint64_t piece_hash = m_piece_hash;
  uint64_t pawn_hash = m_pawn_hash;

  // game_history.erase(board_hash); /* remove the board hash from the game history */

  int from = move.from();
  int to = move.to();
  int type = move.type();

  piece moving_piece = m_sq_board[to];
  piece captured_piece = EMPTY;
  piece promo_piece = EMPTY;
  int opponent_pawn_sq;
  uint64_t from_zobrist = m_hasher.get_hash_val(moving_piece, from);
  uint64_t to_zobrist = m_hasher.get_hash_val(moving_piece, to);
  switch (type) {
    case Move::QUIET_MOVE:
      /* the moving piece will always be the same piece unless we are dealing with a promotion */
      moving_piece = m_sq_board[to];
      place_piece(moving_piece, from);
      remove_piece(moving_piece, to);

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) {
        pawn_hash ^= from_zobrist;
        pawn_hash ^= to_zobrist;
      }
      break;
    case Move::DOUBLE_PUSH:
      moving_piece = m_sq_board[to];
      place_piece(moving_piece, from);
      remove_piece(moving_piece, to);

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      pawn_hash ^= from_zobrist;
      pawn_hash ^= to_zobrist;
      break;
    case Move::KING_SIDE_CASTLE:
      moving_piece = m_sq_board[to];
      place_piece(moving_piece, from);
      remove_piece(moving_piece, to);
      
      board_hash ^= from_zobrist; /* remove the castled king from hash value */
      board_hash ^= to_zobrist; /* place the uncastled king in hash value */
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;

      if(from == constants::E1) { // white king side
        remove_piece(WHITE | ROOK, constants::F1);
        place_piece(WHITE | ROOK, constants::H1);
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::F1); // remove white rook from F1
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::H1); // place white rook on H1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::F1); // remove white rook from F1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::H1); // place white rook on H1
        m_positional_score -= constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::F1];
        m_positional_score += constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::H1];
      }
      else { // black king side
        remove_piece(BLACK | ROOK, constants::F8);
        place_piece(BLACK | ROOK, constants::H8);
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::F8); // remove black rook from F8
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::H8); // place black rook on H8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::F8); // remove black rook from F8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::H8); // place black rook on H8
        m_positional_score -= constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::F8];
        m_positional_score += constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::H8];
      }
      break;
    case Move::QUEEN_SIDE_CASTLE:
      moving_piece = m_sq_board[to];
      place_piece(moving_piece, from);
      remove_piece(moving_piece, to);
      
      board_hash ^= from_zobrist; /* remove the castled king from hash value */
      board_hash ^= to_zobrist; /* place the uncastled king in hash value */
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;

      if(from == constants::E1) { // white queen side
        remove_piece(WHITE | ROOK, constants::D1);
        place_piece(WHITE | ROOK, constants::A1);
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::D1); // remove white rook from D1
        board_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::A1); // place white rook on A1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::D1); // remove white rook from D1
        piece_hash ^= m_hasher.get_hash_val(WHITE | ROOK, constants::A1); // place white rook on A1
        m_positional_score -= constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::D1];
        m_positional_score += constants::piece_scores[constants::WHITE_ROOKS_INDEX][constants::A1];
      }
      else { // black queen side
        remove_piece(BLACK | ROOK, constants::D8);
        place_piece(BLACK | ROOK, constants::A8);
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::D8); // remove black rook from D8
        board_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::A8); // place black rook on A8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::D8); // remove black rook from D8
        piece_hash ^= m_hasher.get_hash_val(BLACK | ROOK, constants::A8); // place black rook on A8
        m_positional_score -= constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::D8];
        m_positional_score += constants::piece_scores[constants::BLACK_ROOKS_INDEX][constants::A8];
      }
      break;
    case Move::NORMAL_CAPTURE:
      moving_piece = m_sq_board[to];
      place_piece(moving_piece, from);
      remove_piece(moving_piece, to);

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) {
        pawn_hash ^= from_zobrist;
        pawn_hash ^= to_zobrist;
      }

      /* place the captured piece back where it was captured from */
      captured_piece = state.get_last_capture();
      place_piece(captured_piece, to);

      board_hash ^= m_hasher.get_hash_val(captured_piece, to); /* place the captured piece in the hash value */
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      if(PIECE(captured_piece) == PAWN)
        pawn_hash ^= m_hasher.get_hash_val(captured_piece, to);
      break;
    case Move::EN_PASSANT_CAPTURE:
      moving_piece = m_sq_board[to];
      place_piece(moving_piece, from);
      remove_piece(moving_piece, to);

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      pawn_hash ^= from_zobrist;
      pawn_hash ^= to_zobrist;

      /* distinguish between white and black en passant */
      opponent_pawn_sq = (utils::rank(to) == constants::RANK_6) ? (to - 8) : (to + 8);

      /* place the captured pawn */
      captured_piece = state.get_last_capture();
      place_piece(captured_piece, opponent_pawn_sq);
      
      board_hash ^= m_hasher.get_hash_val(captured_piece, opponent_pawn_sq); // place the captured pawn in hash value
      piece_hash ^= m_hasher.get_hash_val(captured_piece, opponent_pawn_sq);
      pawn_hash ^= m_hasher.get_hash_val(captured_piece, opponent_pawn_sq);
      break;
    case Move::KNIGHT_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = state.get_last_capture();
      place_piece(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); /* place captured piece back in hash value */
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::KNIGHT_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(!m_white_turn) {
        moving_piece = WHITE | PAWN; 
        promo_piece = WHITE | KNIGHT; 
      }
      else {
        moving_piece = BLACK | PAWN; 
        promo_piece = BLACK | KNIGHT; 
      }
      place_piece(moving_piece, from);

      board_hash ^= m_hasher.get_hash_val(moving_piece, from);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to);
      piece_hash ^= m_hasher.get_hash_val(moving_piece, from);
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      pawn_hash ^= m_hasher.get_hash_val(moving_piece, from);

      remove_piece_from_bb(promo_piece, to);
      if(!move.is_capture()) /* if they aren't capturing anything, then make the promotion square empty */
        m_sq_board[to] = EMPTY;
      break;
    case Move::BISHOP_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = state.get_last_capture();
      place_piece(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); /* place captured piece back in hash value */
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::BISHOP_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(!m_white_turn) {
        moving_piece = WHITE | PAWN; 
        promo_piece = WHITE | BISHOP; 
      }
      else {
        moving_piece = BLACK | PAWN; 
        promo_piece = BLACK | BISHOP; 
      }
      place_piece(moving_piece, from); // place the pawn back 
      board_hash ^= m_hasher.get_hash_val(moving_piece, from);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to);
      piece_hash ^= m_hasher.get_hash_val(moving_piece, from);
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      pawn_hash ^= m_hasher.get_hash_val(moving_piece, from);

      remove_piece_from_bb(promo_piece, to);
      if(!move.is_capture()) /* if they aren't capturing anything, then make the promotion square empty */
        m_sq_board[to] = EMPTY;
      break;
    case Move::ROOK_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = state.get_last_capture();
      place_piece(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); /* place captured piece back in hash value */
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::ROOK_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(!m_white_turn) {
        moving_piece = WHITE | PAWN; 
        promo_piece = WHITE | ROOK; 
      }
      else {
        moving_piece = BLACK | PAWN; 
        promo_piece = BLACK | ROOK; 
      }
      place_piece(moving_piece, from); // place back the pawn 
      board_hash ^= m_hasher.get_hash_val(moving_piece, from);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to);
      piece_hash ^= m_hasher.get_hash_val(moving_piece, from);
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      pawn_hash ^= m_hasher.get_hash_val(moving_piece, from);

      remove_piece_from_bb(promo_piece, to);
      if(!move.is_capture()) /* if they aren't capturing anything, then make the promotion square empty */
        m_sq_board[to] = EMPTY;
      break;
    case Move::QUEEN_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = state.get_last_capture();
      place_piece(captured_piece, to);
      board_hash ^= m_hasher.get_hash_val(captured_piece, to); /* place captured piece back in hash value */
      piece_hash ^= m_hasher.get_hash_val(captured_piece, to);
      /* fallthrough */
    case Move::QUEEN_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(!m_white_turn) {
        moving_piece = WHITE | PAWN; 
        promo_piece = WHITE | QUEEN; 
      }
      else {
        moving_piece = BLACK | PAWN; 
        promo_piece = BLACK | QUEEN; 
      }
      place_piece(moving_piece, from); // place back the pawn
      board_hash ^= m_hasher.get_hash_val(moving_piece, from);
      board_hash ^= m_hasher.get_hash_val(promo_piece, to);
      piece_hash ^= m_hasher.get_hash_val(moving_piece, from);
      piece_hash ^= m_hasher.get_hash_val(promo_piece, to);
      pawn_hash ^= m_hasher.get_hash_val(moving_piece, from);

      remove_piece_from_bb(promo_piece, to);
      if(!move.is_capture()) /* if they aren't capturing anything, then make the promotion square empty */
        m_sq_board[to] = EMPTY;
      break;
  }

  if(moving_piece == (WHITE | KING))
    m_white_king_loc = from;
  else if(moving_piece == (BLACK | KING))
    m_black_king_loc = from;

  board_hash ^= m_hasher.get_black_to_move_hash();

  m_irr_state_history.pop_back(); // go back to previous board's irreversible state
  IrreversibleState prev_state = m_irr_state_history.back();

  /* update hash value castling rights */
  if(!m_white_turn) { // castling rights have changed
    if(prev_state.can_white_king_side_castle() && !state.can_white_king_side_castle())
      board_hash ^= m_hasher.get_white_king_side_hash();
    if(prev_state.can_white_queen_side_castle() && !state.can_white_queen_side_castle())
      board_hash ^= m_hasher.get_white_queen_side_hash();
  }
  else { 
    if(prev_state.can_black_king_side_castle() && !state.can_black_king_side_castle())
      board_hash ^= m_hasher.get_black_king_side_hash();
    if(prev_state.can_black_queen_side_castle() && !state.can_black_queen_side_castle())
      board_hash ^= m_hasher.get_black_queen_side_hash();
  }

  /* update en passant file in hash value */
  int prev_en_passant_sq = prev_state.get_en_passant_sq();
  int curr_en_passant_sq = state.get_en_passant_sq();
  if(prev_en_passant_sq != constants::NONE)
    board_hash ^= m_hasher.get_en_passant_hash(prev_en_passant_sq); // remove the last board's en passant from hash value
  if(curr_en_passant_sq != constants::NONE)
    board_hash ^= m_hasher.get_en_passant_hash(curr_en_passant_sq); // place the current en passant file in hash value

  /* update evaluation items */
  if(captured_piece != EMPTY){
    if(type == Move::EN_PASSANT_CAPTURE)
      m_positional_score += constants::piece_scores[utils::index_from_pc(captured_piece)][opponent_pawn_sq];
    else
      m_positional_score += constants::piece_scores[utils::index_from_pc(captured_piece)][to];
    m_material_score += constants::piece_values[utils::index_from_pc(captured_piece)];
    m_piece_counts[utils::index_from_pc(captured_piece)]++;
    m_total_material += abs(constants::piece_values[utils::index_from_pc(captured_piece)]);
  }

  if(promo_piece != EMPTY) {
    if(COLOR(promo_piece) == WHITE) {
      m_material_score += constants::piece_values[constants::WHITE_PAWNS_INDEX];
      m_piece_counts[constants::WHITE_PAWNS_INDEX]++;
      m_positional_score += constants::piece_scores[constants::WHITE_PAWNS_INDEX][from];
      m_total_material += abs(constants::piece_values[constants::WHITE_PAWNS_INDEX]);
    }
    else {
      m_material_score += constants::piece_values[constants::BLACK_PAWNS_INDEX];
      m_piece_counts[constants::BLACK_PAWNS_INDEX]++;
      m_positional_score += constants::piece_scores[constants::BLACK_PAWNS_INDEX][from];
      m_total_material += abs(constants::piece_values[constants::BLACK_PAWNS_INDEX]);
    }
    m_material_score -= constants::piece_values[utils::index_from_pc(promo_piece)];
    m_positional_score -= constants::piece_scores[utils::index_from_pc(promo_piece)][to];
    m_piece_counts[utils::index_from_pc(promo_piece)]--;
    m_total_material -= abs(constants::piece_values[utils::index_from_pc(promo_piece)]);
  }
  else if(PIECE(moving_piece) != KING) {
    m_positional_score -= constants::piece_scores[utils::index_from_pc(moving_piece)][to];
    m_positional_score += constants::piece_scores[utils::index_from_pc(moving_piece)][from];
  }

  m_white_turn = !m_white_turn;
  m_board_hash = board_hash;
  m_piece_hash = piece_hash;
  m_pawn_hash = pawn_hash;
  update_redundant_boards();
}

void Board::make_nullmove() {
  IrreversibleState prev_state = m_irr_state_history.back();
  IrreversibleState state = prev_state;
  int prev_en_passant = prev_state.get_en_passant_sq();
  if(prev_en_passant != constants::NONE)
    m_board_hash ^= m_hasher.get_en_passant_hash(prev_en_passant);
  state.set_en_passant_sq(constants::NONE);
  state.set_last_move(Move::NO_MOVE);
  m_white_turn = !m_white_turn;
  m_board_hash ^= m_hasher.get_black_to_move_hash();
  m_irr_state_history.push_back(state);
}

void Board::unmake_nullmove() {
  m_irr_state_history.pop_back();
  IrreversibleState state = m_irr_state_history.back();
  int en_passant_sq = state.get_en_passant_sq();
  if(en_passant_sq != constants::NONE)
    m_board_hash ^= m_hasher.get_en_passant_hash(en_passant_sq);
  m_white_turn = !m_white_turn;
  m_board_hash ^= m_hasher.get_black_to_move_hash();
}

void Board::make_moves(std::vector<Move> moves)
{
  for (auto move : moves)
  {
    make_move(move);
  }
}

void Board::place_piece(piece pc, int sq)
{
  if (sq == constants::NONE)
  {
    std::cerr << "Attempting to place piece on NONE square" << std::endl;
  }

  m_sq_board[sq] = pc;
  place_piece_in_bb(pc, sq);
}

void Board::place_piece_in_bb(piece pc, int sq)
{
  int index = utils::index_from_pc(pc);
  m_piece_boards[index] |= (1 << sq);
}

void Board::remove_piece(piece pc, int sq)
{
  if (sq == constants::NONE)
  {
    std::cerr << "Attempting to remove piece from NONE square" << std::endl;
  }

  m_sq_board[sq] = EMPTY;
  remove_piece_from_bb(pc, sq);
}

void Board::remove_piece_from_bb(piece pc, int sq)
{
  int index = utils::index_from_pc(pc);
  m_piece_boards[index] &= ~(1 << sq);
}

void Board::update_redundant_boards()
{
  m_white_pieces = (m_piece_boards[constants::WHITE_PAWNS_INDEX]   | m_piece_boards[constants::WHITE_KNIGHTS_INDEX] | 
                    m_piece_boards[constants::WHITE_BISHOPS_INDEX] | m_piece_boards[constants::WHITE_ROOKS_INDEX]   |
                    m_piece_boards[constants::WHITE_QUEENS_INDEX]  | m_piece_boards[constants::WHITE_KINGS_INDEX]);

  m_black_pieces = (m_piece_boards[constants::BLACK_PAWNS_INDEX]   | m_piece_boards[constants::BLACK_KNIGHTS_INDEX] | 
                    m_piece_boards[constants::BLACK_BISHOPS_INDEX] | m_piece_boards[constants::BLACK_ROOKS_INDEX]   |
                    m_piece_boards[constants::BLACK_QUEENS_INDEX]  | m_piece_boards[constants::BLACK_KINGS_INDEX]);

  m_all_pieces = m_white_pieces | m_black_pieces;
}

Board::IrreversibleState::IrreversibleState(bool white_ks, 
                                            bool white_qs, 
                                            bool black_ks, 
                                            bool black_qs, 
                                            int en_passant_sq, 
                                            piece last_capture, 
                                            uint16_t fifty_move_count,
                                            uint32_t irr_ply,
                                            Move last_move)
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

///////////////////////////////////////////////// GETTER FUNCTIONS /////////////////////////////////////////////////

piece Board::operator[](int sq) const
{
  return m_sq_board[sq];
}

int Board::get_piece_count(piece pc) const
{
  return m_piece_counts[utils::index_from_pc(pc)];
}

int Board::get_material_score() const
{
  return m_material_score;
}

int Board::get_positional_score() const
{
  return m_positional_score;
}

int Board::get_total_material() const
{
  return m_total_material;
}

uint64_t Board::get_hash() const
{
  return m_board_hash;
}

uint64_t Board::get_piece_hash() const
{
  return m_piece_hash;
}

uint64_t Board::get_pawn_hash() const
{
  return m_pawn_hash;
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

bitboard Board::get_piece_bitboard(piece pc) const
{
  return m_piece_boards[utils::index_from_pc(pc)];
}

int Board::get_white_king_loc() const
{
  return m_white_king_loc;
}

int Board::get_black_king_loc() const
{
  return m_black_king_loc;
}

bool Board::can_white_king_side_castle() const
{
  return m_irr_state_history.back().can_white_king_side_castle();
}

bool Board::can_white_queen_side_castle() const
{
  return m_irr_state_history.back().can_white_queen_side_castle();
}

bool Board::can_black_king_side_castle() const
{
  return m_irr_state_history.back().can_black_king_side_castle();
}

bool Board::can_black_queen_side_castle() const
{
  return m_irr_state_history.back().can_black_queen_side_castle();
}

int Board::get_en_passant_sq() const
{
  return m_irr_state_history.back().get_en_passant_sq();
}

Move Board::get_last_move() const
{
  return m_irr_state_history.back().get_last_move();
}

bitboard Board::get_white_pieces() const
{
  return m_white_pieces;
}

bitboard Board::get_black_pieces() const
{
  return m_black_pieces;
}
bitboard Board::get_all_pieces() const
{
  return m_all_pieces;
}

bool Board::is_white_turn() const
{
  return m_white_turn;
}

///////////////////////////////////////////////// HELPFUL BOARD FUNCTIONS /////////////////////////////////////////////////

std::string Board::to_string() const
{
  char c;
  piece pc;
  std::string str;
  for (size_t i = 0; i < 8; i++) {
    for (size_t j = 0; j < 8; j++) {
      pc = m_sq_board[(7-i)*8 + j];
      switch (pc) {
        case (WHITE | PAWN) :
          c = 'P';
          break;
        case (BLACK | PAWN) :
          c = 'p';
          break;
        case (WHITE | KNIGHT) :
          c = 'N';
          break;
        case (BLACK | KNIGHT) :
          c = 'n';
          break;
        case (WHITE | BISHOP) :
          c = 'B';
          break;
        case (BLACK | BISHOP) :
          c = 'b';
          break;
        case (WHITE | ROOK) :
          c = 'R';
          break;
        case (BLACK | ROOK) :
          c = 'r';
          break;
        case (WHITE | QUEEN) :
          c = 'Q';
          break;
        case (BLACK | QUEEN) :
          c = 'q';
          break;
        case (WHITE | KING) :
          c = 'K';
          break;
        case (BLACK | KING) :
          c = 'k';
          break;
        default:
          c = '*';
          break;
      }
      str.push_back(c);
    }
    str.push_back('\n');
  }
  str.push_back('\n');
  return str;
}