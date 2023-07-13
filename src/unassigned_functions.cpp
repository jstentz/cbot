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