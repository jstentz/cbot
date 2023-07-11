bitboard generate_attack_map(board_t board, turn side) {
  bitboard attack_map = 0;
  size_t pc_loc;
  bitboard color_pieces;
  bitboard all_pieces_no_king;
  piece curr_piece;
  piece color;
  if(side == W) {
    all_pieces_no_king = board.all_pieces & 
              ~board.piece_boards[BLACK_KINGS_INDEX];
    color_pieces = board.white_pieces;
    color = WHITE;
  }
  else {
    all_pieces_no_king = board.all_pieces & 
              ~board.piece_boards[WHITE_KINGS_INDEX];
    color_pieces = board.black_pieces;
    color = BLACK;
  }

  while(color_pieces) {
    pc_loc = first_set_bit(color_pieces);
    curr_piece = board.sq_board[pc_loc];
    if(curr_piece == (color | PAWN)) {
      attack_map |= get_pawn_attacks((square)pc_loc, side);
    }
    else if(curr_piece == (color | KNIGHT)) {
      attack_map |= get_knight_attacks((square)pc_loc);
    }
    else if(curr_piece == (color | BISHOP)) {
      attack_map |= get_bishop_attacks((square)pc_loc, all_pieces_no_king);
    }
    else if(curr_piece == (color | ROOK)) {
      attack_map |= get_rook_attacks((square)pc_loc, all_pieces_no_king);
    }
    else if(curr_piece == (color | QUEEN)) {
      attack_map |= get_queen_attacks((square)pc_loc, all_pieces_no_king);
    }
    else if(curr_piece == (color | KING)) {
      attack_map |= get_king_attacks((square)pc_loc);
    }
    REMOVE_FIRST(color_pieces);
  }
  return attack_map;
}

// this function doesnt include the king
// I should have this also return the location of the least valued attacker
// in order to construct the move for SEE
piece least_valued_attacker(square sq) {
  bitboard opponent_knights;
  // bitboard opponent_kings;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(b.t == W) {
    opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
    // opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
    opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
  }
  else {
    opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
    // opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
    opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
  }
  if(get_pawn_attacks(sq, b.t) & opponent_pawns) return PAWN;
  if(get_knight_attacks(sq) & opponent_knights) return KNIGHT;
  if(get_bishop_attacks(sq, b.all_pieces) & opponent_bishops) return BISHOP;
  if(get_rook_attacks(sq, b.all_pieces) & opponent_rooks) return ROOK;
  if(get_queen_attacks(sq, b.all_pieces) & opponent_queens) return QUEEN;
  // if(get_king_attacks(sq) & opponent_kings) return KING; /* not sure what to do here */
  return EMPTY;
}
/* here the side is the person being attacked */
square least_valued_attacker_sq(square sq, turn side) {
  bitboard opponent_knights;
  // bitboard opponent_kings;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(side == W) {
    opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
    // opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
    opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
  }
  else {
    opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
    // opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
    opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
  }
  bitboard pawns = get_pawn_attacks(sq, side) & opponent_pawns;
  if(pawns) return (square)first_set_bit(pawns);

  bitboard knights = get_knight_attacks(sq) & opponent_knights;
  if(knights) return (square)first_set_bit(knights);

  bitboard bishops = get_bishop_attacks(sq, b.all_pieces) & opponent_bishops;
  if(bishops) return (square)first_set_bit(bishops);

  bitboard rooks = get_rook_attacks(sq, b.all_pieces) & opponent_rooks;
  if(rooks) return (square)first_set_bit(rooks);

  bitboard queens = get_queen_attacks(sq, b.all_pieces) & opponent_queens;
  if(queens) return (square)first_set_bit(queens);
  return NONE;
}

bool is_attacked_by_pawn(square sq) {
  bitboard opponent_pawns;
  if(b.t == W)
    opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
  else
    opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
  if(get_pawn_attacks(sq, b.t) & opponent_pawns) return true;
  return false;
}

/* this function is for pieces only */
bool is_attacked(square sq, bitboard blocking_pieces) {
  bitboard opponent_knights;
  bitboard opponent_kings;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(b.t == W) {
    opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
    opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
    opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
  }
  else {
    opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
    opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
    opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
  }
  if(get_bishop_attacks(sq, blocking_pieces) & opponent_bishops) return true;
  if(get_rook_attacks(sq, blocking_pieces) & opponent_rooks) return true;
  if(get_knight_attacks(sq) & opponent_knights) return true;
  if(get_pawn_attacks(sq, b.t) & opponent_pawns) return true;
  if(get_queen_attacks(sq, blocking_pieces) & opponent_queens) return true;
  if(get_king_attacks(sq) & opponent_kings) return true;
  return false;
}

bitboard attackers_from_square(square sq) {
  bitboard attackers = 0;
  bitboard opponent_knights;
  bitboard opponent_kings;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(b.t == W) {
    opponent_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
    opponent_kings = b.piece_boards[BLACK_KINGS_INDEX];
    opponent_pawns = b.piece_boards[BLACK_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
  }
  else {
    opponent_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
    opponent_kings = b.piece_boards[WHITE_KINGS_INDEX];
    opponent_pawns = b.piece_boards[WHITE_PAWNS_INDEX];
    opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
  }
  attackers |= get_knight_attacks(sq) & opponent_knights;
  attackers |= get_king_attacks(sq) & opponent_kings;
  attackers |= get_pawn_attacks(sq, b.t) & opponent_pawns;
  attackers |= get_rook_attacks(sq, b.all_pieces) & opponent_rooks;
  attackers |= get_bishop_attacks(sq, b.all_pieces) & opponent_bishops;
  attackers |= get_queen_attacks(sq, b.all_pieces) & opponent_queens;
  return attackers;
}

bitboard opponent_slider_rays_to_square(square sq) {
  bitboard res = 0;
  square attacker_loc;
  bitboard attackers;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(b.t == W) {
    opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
  }
  else {
    opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
    opponent_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
    opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
  }
  bitboard rook_attacks_from_sq = get_rook_attacks(sq, b.all_pieces);
  attackers = rook_attacks_from_sq & (opponent_rooks | opponent_queens);
  while(attackers) {
    attacker_loc = (square)first_set_bit(attackers);
    res |= get_rook_attacks(attacker_loc, b.all_pieces) & rook_attacks_from_sq;
    REMOVE_FIRST(attackers);    
  }
  bitboard bishop_attacks_from_sq = get_bishop_attacks(sq, b.all_pieces);
  attackers = bishop_attacks_from_sq & (opponent_bishops | opponent_queens);
  while(attackers) {
    attacker_loc = (square)first_set_bit(attackers);
    res |= get_bishop_attacks(attacker_loc, b.all_pieces) & bishop_attacks_from_sq;
    REMOVE_FIRST(attackers);
  }
  return res;
}

struct Pin {
  bitboard ray_at_sq[64];
  bitboard pinned_pieces{};
};

/// TODO: this is actually appauling please fix this function (might need to rework how I do pins)
/// TODO: move this function into the move generator and out of the board
Board::Pin get_pinned_pieces(int friendly_king_loc) const 
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
  int king_rank = utils::rank(friendly_king_loc);
  int king_file = utils::file(friendly_king_loc);
  int king_diag = king_rank - king_file;
  int king_antidiag = king_rank + king_file;
  while(opponent_rooks) {
    pc_loc = first_set_bit(opponent_rooks);
    pc_rank = utils::rank(pc_loc);
    pc_file = utils::file(pc_loc);
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
  Move last_move = m_irr_state_history.back().get_last_move();
  if(last_move.get_move() == Move::NO_MOVE) {
    return attackers_from_square(friendly_king);
  }
  bool discovered_check = false;

  int to = last_move.to();
  int from = last_move.from();
  int type = last_move.type();

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
  if(type == Move::QUIET_MOVE || type == Move::DOUBLE_PUSH || type == Move::NORMAL_CAPTURE) {
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