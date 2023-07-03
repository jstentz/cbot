#include "include/moves.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/attacks.h"
#include "include/pieces.h"
#include "include/evaluation.h"
#include "include/hashing.h"
#include "include/debugging.h"
#include "include/tt.h"

#include <vector>
#include <stack>
#include <iostream>
#include <string>
#include <algorithm>

bitboard generate_knight_move_bitboard(square knight, bool captures_only) {
  bitboard knight_attacks = luts.knight_attacks[knight];
  if(captures_only) {
    bitboard opponent_pieces = (b.t == W) ? b.black_pieces : b.white_pieces;
    return knight_attacks & opponent_pieces;
  }
  bitboard own_pieces = (b.t == W) ? b.white_pieces : b.black_pieces;
  return knight_attacks & ~own_pieces;
}

bitboard generate_king_move_bitboard(square king, bool captures_only) {
  bitboard own_pieces = (b.t == W) ? b.white_pieces : b.black_pieces;
  bitboard opponent_pieces = (b.t == W) ? b.black_pieces : b.white_pieces;
  
  bitboard king_attacks = luts.king_attacks[king];
  bitboard king_pseudomoves = (captures_only) ? (king_attacks & opponent_pieces) : (king_attacks & ~own_pieces);

  if(!king_pseudomoves) return 0; // if the king has no pseudolegal moves, it cannot castle

  bitboard king_legal_moves = 0;
  bitboard blocking_pieces = b.all_pieces & ~BIT_FROM_SQ(king); // the king cannot block the attack on a square behind it

  while(king_pseudomoves) {
    square loc = (square)first_set_bit(king_pseudomoves);
    if(!is_attacked(loc, blocking_pieces)) king_legal_moves |= BIT_FROM_SQ(loc);
    REMOVE_FIRST(king_pseudomoves);
  }

  if(captures_only) return king_legal_moves; // you can't castle and capture something

  // generate castling moves
  // maybe move these to defined constants
  
  const bitboard w_king_side_castle = 0x40;
  const bitboard w_queen_side_castle = 0x4;
  const bitboard b_king_side_castle = 0x4000000000000000;
  const bitboard b_queen_side_castle = 0x0400000000000000;

  square white_king_sq_1 = F1;
  square white_king_sq_2 = G1;
  square white_queen_sq_1 = D1;
  square white_queen_sq_2 = C1;
  square white_queen_sq_3 = B1; // this square is allowed to be attacked

  square black_king_sq_1 = F8;
  square black_king_sq_2 = G8;
  square black_queen_sq_1 = D8;
  square black_queen_sq_2 = C8;
  square black_queen_sq_3 = B8; // this square is allowed to be attacked

  bitboard king_castle = 0;
  state_t state = b.state_history.top();
  if(b.t == W && b.white_king_loc == E1 && !is_attacked(E1, blocking_pieces)) {
    if(WHITE_KING_SIDE(state) && b.sq_board[H1] == (WHITE | ROOK)) {
      if(b.sq_board[white_king_sq_1] == EMPTY &&
         b.sq_board[white_king_sq_2] == EMPTY) {
           if(!is_attacked(white_king_sq_1, blocking_pieces) &&
            !is_attacked(white_king_sq_2, blocking_pieces))
            king_castle |= w_king_side_castle;
         }
    }

    if(WHITE_QUEEN_SIDE(state) && b.sq_board[A1] == (WHITE | ROOK)) {
      if(b.sq_board[white_queen_sq_1] == EMPTY &&
         b.sq_board[white_queen_sq_2] == EMPTY &&
         b.sq_board[white_queen_sq_3] == EMPTY) {
           if(!is_attacked(white_queen_sq_1, blocking_pieces) &&
            !is_attacked(white_queen_sq_2, blocking_pieces))
            king_castle |= w_queen_side_castle;
         }
    } 
  }
  else if (b.t == B && b.black_king_loc == E8 && !is_attacked(E8, blocking_pieces)) {
    if(BLACK_KING_SIDE(state) && b.sq_board[H8] == (BLACK | ROOK)) {
      if(b.sq_board[black_king_sq_1] == EMPTY &&
         b.sq_board[black_king_sq_2] == EMPTY) {
           if(!is_attacked(black_king_sq_1, blocking_pieces) &&
            !is_attacked(black_king_sq_2, blocking_pieces))
            king_castle |= b_king_side_castle;
         }
    }

    if(BLACK_QUEEN_SIDE(state) && b.sq_board[A8] == (BLACK | ROOK)) {
      if(b.sq_board[black_queen_sq_1] == EMPTY &&
         b.sq_board[black_queen_sq_2] == EMPTY &&
         b.sq_board[black_queen_sq_3] == EMPTY) {
           if(!is_attacked(black_queen_sq_1, blocking_pieces) &&
            !is_attacked(black_queen_sq_2, blocking_pieces))
            king_castle |= b_queen_side_castle;
         }
    } 
  }
  return king_legal_moves | king_castle;           
}

bitboard generate_pawn_move_bitboard(square pawn, bool captures_only) {
  state_t state = b.state_history.top();
  bitboard enemy_pieces;
  bitboard all_pieces = b.all_pieces;
  bitboard captures;
  bitboard forward_moves;
  bitboard forward_one;
  bitboard forward_two;
  bitboard en_passant_capture;
  square en_passant_sq = (square)EN_PASSANT_SQ(state);
  bitboard en_passant_bit = 0; // default it to zero
  size_t rank = RANK(pawn);
  bitboard opponent_rooks;
  bitboard opponent_queens;
  bitboard attackers;
  bitboard side_attackers;
  bitboard board_without_pawns;
  bitboard white_pawn_attacks;
  bitboard black_pawn_attacks;

  if(en_passant_sq != NONE) {
    en_passant_bit =  BIT_FROM_SQ(en_passant_sq); // used to and with attack pattern
  }

  if(b.t == W) {
    enemy_pieces = b.black_pieces;
    white_pawn_attacks = luts.white_pawn_attacks[pawn];
    captures = white_pawn_attacks & enemy_pieces;

    en_passant_capture = white_pawn_attacks & en_passant_bit;
    if(en_passant_capture && rank == RANK(b.white_king_loc)){
      opponent_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
      opponent_queens = b.piece_boards[BLACK_QUEENS_INDEX];
      board_without_pawns = b.all_pieces & ~(BIT_FROM_SQ(pawn)) & ~(en_passant_bit >> 8);
      attackers = get_rook_attacks(b.white_king_loc, board_without_pawns) & (opponent_rooks | opponent_queens);
      side_attackers = attackers & luts.mask_rank[RANK(pawn)];
      if(side_attackers) {
        en_passant_capture = 0;
      }
    }

    if(captures_only) return captures | en_passant_capture;

    forward_one = luts.white_pawn_pushes[pawn] & ~all_pieces;
    forward_two = 0;
    if(rank == RANK_2 && forward_one) {
      forward_two = luts.white_pawn_pushes[pawn + 8] & ~all_pieces;
    }
    forward_moves = forward_one | forward_two;
  }
  else {
    enemy_pieces = b.white_pieces;
    black_pawn_attacks = luts.black_pawn_attacks[pawn];
    captures = black_pawn_attacks & enemy_pieces;

    en_passant_capture = black_pawn_attacks & en_passant_bit;
    if(en_passant_capture && rank == RANK(b.black_king_loc)){
      opponent_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
      opponent_queens = b.piece_boards[WHITE_QUEENS_INDEX];
      board_without_pawns = b.all_pieces & ~(BIT_FROM_SQ(pawn)) & ~(en_passant_bit << 8);
      attackers = get_rook_attacks(b.black_king_loc, board_without_pawns) & (opponent_rooks | opponent_queens);
      side_attackers = attackers & luts.mask_rank[RANK(pawn)];
      if(side_attackers) {
        en_passant_capture = 0;
      }
    }

    if(captures_only) return captures | en_passant_capture;

    forward_one = luts.black_pawn_pushes[pawn] & ~all_pieces;
    forward_two = 0;
    if(rank == RANK_7 && forward_one) {
      forward_two = luts.black_pawn_pushes[pawn - 8] & ~all_pieces;
    }
    forward_moves = forward_one | forward_two;
  }

  return captures | forward_moves | en_passant_capture;
}

bitboard generate_rook_move_bitboard(square rook, bool captures_only) {
  bitboard rook_attacks = get_rook_attacks(rook, b.all_pieces);
  if(captures_only) {
    bitboard opponent_pieces = (b.t == W) ? b.black_pieces : b.white_pieces;
    return rook_attacks & opponent_pieces;
  }
  bitboard own_pieces = (b.t == W) ? b.white_pieces : b.black_pieces;
  return rook_attacks & ~own_pieces;
}

bitboard generate_bishop_move_bitboard(square bishop, bool captures_only) {
  bitboard bishop_attacks = get_bishop_attacks(bishop, b.all_pieces);
  if(captures_only) {
    bitboard opponent_pieces = (b.t == W) ? b.black_pieces : b.white_pieces;
    return bishop_attacks & opponent_pieces;
  }
  bitboard own_pieces = (b.t == W) ? b.white_pieces : b.black_pieces;
  return bishop_attacks & ~own_pieces;
}

bitboard generate_queen_move_bitboard(square queen, bool captures_only) {
  return   generate_rook_move_bitboard(queen, captures_only)
       | generate_bishop_move_bitboard(queen, captures_only);
}

move_t construct_move(int from, int to, int flags) {
  return (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12);
}

void generate_knight_moves(std::vector<move_t> &curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard knights;
  bitboard opponent_pieces;
  if (b.t == W) {
    knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
    opponent_pieces = b.black_pieces;
  }
  else {
    knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
    opponent_pieces = b.white_pieces;
  }
  bitboard knight_moves;
  bitboard knight_bit;
  while(knights) {
    from = first_set_bit(knights);
    knight_bit = BIT_FROM_SQ(from);
    if(knight_bit & pin->pinned_pieces) {
      REMOVE_FIRST(knights);
      continue;
    } // pinned knights cannot move at all
    knight_moves = generate_knight_move_bitboard((square)from, captures_only) & check_mask;
    
    while(knight_moves) {
      to = first_set_bit(knight_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
      else                         flags = QUIET_MOVE;
      curr_moves.push_back(construct_move(from, to, flags));
      REMOVE_FIRST(knight_moves);
    }
    REMOVE_FIRST(knights);
  }
  return;
}

void generate_king_moves(std::vector<move_t> &curr_moves, bool captures_only) {
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard kings;
  bitboard opponent_pieces;
  if (b.t == W) {
    kings = b.piece_boards[WHITE_KINGS_INDEX];
    opponent_pieces = b.black_pieces;
  }
  else {
    kings = b.piece_boards[BLACK_KINGS_INDEX];
    opponent_pieces = b.white_pieces;
  }
  bitboard king_moves;
  while(kings) {
    from = first_set_bit(kings);
    king_moves = generate_king_move_bitboard((square)from, captures_only);
    while(king_moves) {
      to = first_set_bit(king_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to - from == 2) { // king side castle
        curr_moves.push_back(construct_move(from, to, KING_SIDE_CASTLE));
      }
      else if (to - from == -2) { // queen side castle
        curr_moves.push_back(construct_move(from, to, QUEEN_SIDE_CASTLE));
      } 
      else{
        if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
        else                         flags = QUIET_MOVE;
        curr_moves.push_back(construct_move(from, to, flags));
      }
      
      REMOVE_FIRST(king_moves);
    }
    REMOVE_FIRST(kings);
  }
  return;
}

void generate_pawn_moves(std::vector<move_t> &curr_moves, bitboard check_mask, bool pawn_check, pin_t *pin, bool captures_only) {
  state_t state = b.state_history.top();
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard pawns;
  bitboard opponent_pieces; // used for captures only
  if (b.t == W) {
    pawns = b.piece_boards[WHITE_PAWNS_INDEX];
    opponent_pieces = b.black_pieces;
  }
  else {
    pawns = b.piece_boards[BLACK_PAWNS_INDEX];
    opponent_pieces = b.white_pieces;
  }

  bitboard pawn_moves;
  bitboard pawn_bit;
  bitboard en_passant_bit = 0;
  if(EN_PASSANT_SQ(state) != NONE) {
    en_passant_bit = BIT_FROM_SQ(EN_PASSANT_SQ(state));
    if(pawn_check) {
      check_mask |= en_passant_bit;
    }
  }
  while(pawns) {
    from = first_set_bit(pawns);
    pawn_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(pawn_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
    pawn_moves = generate_pawn_move_bitboard((square)from, captures_only) & check_mask & pin_mask;
    
    while(pawn_moves) {
      to = first_set_bit(pawn_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & en_passant_bit)         flags = EN_PASSANT_CAPTURE;
      else if(to_bit & opponent_pieces)   flags = NORMAL_CAPTURE;
      else                                flags = QUIET_MOVE;
      int to_rank = RANK(to);
      if(to_rank == RANK_8 || to_rank == RANK_1) {
        if(FILE(to) != FILE(from)) {
          curr_moves.push_back(construct_move(from, to, KNIGHT_PROMO_CAPTURE)); // or this on to flag because we check for captures prior to this
          curr_moves.push_back(construct_move(from, to, BISHOP_PROMO_CAPTURE));
          curr_moves.push_back(construct_move(from, to, ROOK_PROMO_CAPTURE));
          curr_moves.push_back(construct_move(from, to, QUEEN_PROMO_CAPTURE));
        }
        else {
          curr_moves.push_back(construct_move(from, to, KNIGHT_PROMO));
          curr_moves.push_back(construct_move(from, to, BISHOP_PROMO));
          curr_moves.push_back(construct_move(from, to, ROOK_PROMO));
          curr_moves.push_back(construct_move(from, to, QUEEN_PROMO));
        }
        
      }
      else if (abs(from - to) == 16) { // double pawn push
        curr_moves.push_back(construct_move(from, to, DOUBLE_PUSH));
      }
      else {
        curr_moves.push_back(construct_move(from, to, flags)); // should already be set to quiet or capture
      }
      REMOVE_FIRST(pawn_moves);
    }
    REMOVE_FIRST(pawns);
  }
  return;
}

void generate_rook_moves(std::vector<move_t> &curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard rooks;
  bitboard opponent_pieces;
  if (b.t == W) {
    rooks = b.piece_boards[WHITE_ROOKS_INDEX];
    opponent_pieces = b.black_pieces;
  }
  else {
    rooks = b.piece_boards[BLACK_ROOKS_INDEX];
    opponent_pieces = b.white_pieces;
  }

  bitboard rook_moves;
  bitboard rook_bit;
  while(rooks) {
    from = first_set_bit(rooks);
    rook_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(rook_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
    rook_moves = generate_rook_move_bitboard((square)from, captures_only) & check_mask & pin_mask;

    while(rook_moves) {
      to = first_set_bit(rook_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
      else                         flags = QUIET_MOVE;
      curr_moves.push_back(construct_move(from, to, flags));
      REMOVE_FIRST(rook_moves);
    }
    REMOVE_FIRST(rooks);
  }
  return;
}

void generate_bishop_moves(std::vector<move_t> &curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard bishops;
  bitboard opponent_pieces;
  if (b.t == W) {
    bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
    opponent_pieces = b.black_pieces;
  }
  else {
    bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
    opponent_pieces = b.white_pieces;
  }

  bitboard bishop_moves;
  bitboard bishop_bit;
  while(bishops) {
    from = first_set_bit(bishops);
    bishop_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(bishop_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
    bishop_moves = generate_bishop_move_bitboard((square)from, captures_only) & check_mask & pin_mask;
    
    while(bishop_moves) {
      to = first_set_bit(bishop_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
      else                         flags = QUIET_MOVE;
      curr_moves.push_back(construct_move(from, to, flags));
      REMOVE_FIRST(bishop_moves);
    }
    REMOVE_FIRST(bishops);
  }
  return;
}

void generate_queen_moves(std::vector<move_t> &curr_moves, bitboard check_mask, pin_t *pin, bool captures_only) {
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard queens;
  bitboard opponent_pieces;
  if (b.t == W) {
    queens = b.piece_boards[WHITE_QUEENS_INDEX];
    opponent_pieces = b.black_pieces;
  }
  else {
    queens = b.piece_boards[BLACK_QUEENS_INDEX];
    opponent_pieces = b.white_pieces;
  }

  bitboard queen_moves;
  bitboard queen_bit;
  while(queens) {
    from = first_set_bit(queens);
    queen_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(queen_bit & pin->pinned_pieces) pin_mask = pin->ray_at_sq[from];
    queen_moves = generate_queen_move_bitboard((square)from, captures_only) & check_mask & pin_mask;
    
    while(queen_moves) {
      to = first_set_bit(queen_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = NORMAL_CAPTURE;
      else                         flags = QUIET_MOVE;
      curr_moves.push_back(construct_move(from, to, flags));
      REMOVE_FIRST(queen_moves);
    }
    REMOVE_FIRST(queens);
  }
  return;
}

void generate_moves(std::vector<move_t> &curr_moves, bool captures_only) {
  bitboard check_pieces = checking_pieces();
  bitboard capture_mask = 0xFFFFFFFFFFFFFFFF;
  bitboard push_mask = 0xFFFFFFFFFFFFFFFF;
  square friendly_king_loc = (b.t == W) ? b.white_king_loc : b.black_king_loc;
  int check = check_type(check_pieces);
  if(check == DOUBLE_CHECK) {
    generate_king_moves(curr_moves, captures_only);
    return;
  }
  else if (check == SINGLE_CHECK) {
    capture_mask = check_pieces;
    square sq = (square)first_set_bit(check_pieces);
    if(is_sliding_piece(b.sq_board[sq])) {
      push_mask = opponent_slider_rays_to_square(friendly_king_loc);
    }
    else {
      push_mask = 0;
    }
  }
  bool pawn_check = (check_pieces & (b.piece_boards[WHITE_PAWNS_INDEX] | b.piece_boards[BLACK_PAWNS_INDEX])) != 0;
  bitboard check_mask = push_mask | capture_mask;
  pin_t pin = get_pinned_pieces(friendly_king_loc); // maybe change this so that the board holds the pinned pieces info
  generate_pawn_moves(curr_moves, check_mask, pawn_check, &pin, captures_only);
  generate_knight_moves(curr_moves, check_mask, &pin, captures_only);
  generate_bishop_moves(curr_moves, check_mask, &pin, captures_only);
  generate_rook_moves(curr_moves, check_mask, &pin, captures_only);
  generate_queen_moves(curr_moves, check_mask, &pin, captures_only);
  generate_king_moves(curr_moves, captures_only);
  return;
}

int see(int sq) {
  int value = 0;
  square attacker_sq = least_valued_attacker_sq((square)sq, !b.t);
  if(attacker_sq != NONE) {
    move_t capture = construct_move(attacker_sq, sq, NORMAL_CAPTURE); /* would this also work for promotions? */
    piece captured_piece = b.sq_board[sq];
    make_move(capture);
    value = std::max(0, abs(piece_values[INDEX_FROM_PIECE(captured_piece)]) - see(sq));
    unmake_move(capture);
  }
  return value;
}

int see_capture(move_t capture) {
  int value = 0;
  int to = TO(capture);
  piece cap_piece = b.sq_board[to];
  make_move(capture);
  value = abs(piece_values[INDEX_FROM_PIECE(cap_piece)]) - see(to);
  unmake_move(capture);
  return value;
}

bool is_bad_capture(move_t capture) {
  piece mv_piece = b.sq_board[FROM(capture)];
  piece cap_piece = b.sq_board[TO(capture)];
  /* if we are capturing a piece of higher material, its probably good */
  if(abs(piece_values[INDEX_FROM_PIECE(cap_piece)]) 
     - abs(piece_values[INDEX_FROM_PIECE(mv_piece)]) > 50) {
    return false;
  }

  return see_capture(capture) < -50;
}

bool pawn_promo_or_close_push(move_t move) {
  if(IS_PROMO(move)) return true;
  int from = FROM(move);
  int to = TO(move);
  piece moving_piece = b.sq_board[from];
  if(PIECE(moving_piece) != PAWN) return false;
  if(RANK(to) == RANK_7 && COLOR(moving_piece) == WHITE) return true;
  if(RANK(to) == RANK_2 && COLOR(moving_piece) == BLACK) return true;
  return false;
}

// thinking about adding en passant to the move
// if you move to a square that is attacked by a lesser-valued piece, put it last
void order_moves(std::vector<move_t> &moves, move_t tt_best_move) {
  state_t state = b.state_history.top();
  signed short int score;
  piece mv_piece;
  piece tar_piece;
  int to;
  int from;
  move_t mv;
  int flags;
  int perspective = (b.t == W) ? 1 : -1;
  // maybe add a bonus for castling moves
  // add recapturing the piece that was last captured as a good bonus to check first
  // bigger bonus for the higher value piece being captured
  // just have the board store the move that was made to get to that position
  // still need to add the least_valued_attacker logic, not exactly sure how to implement
  move_t last_move = LAST_MOVE(state);
  int recapture_square = -1;
  if(last_move != NO_MOVE && IS_CAPTURE(last_move)) {
    recapture_square = TO(last_move);
  }
  for(int i = 0; i < moves.size(); i++) {
    score = 0;
    mv = moves[i];
    if(tt_best_move != NO_MOVE && mv == tt_best_move) {
      // std::cout << "trying pv move" << endl;
      score += 10000; // idk try the PV node first
    }
    to = TO(mv);
    from = FROM(mv);
    flags = FLAGS(mv);
    mv_piece = b.sq_board[from];
    if(IS_PROMO(mv)) {
      if(flags == KNIGHT_PROMO || flags == KNIGHT_PROMO_CAPTURE) {
        score += piece_values[WHITE_KNIGHTS_INDEX]; // just use the white knights because positive value
      }
      else if(flags == BISHOP_PROMO || flags == BISHOP_PROMO_CAPTURE) {
        score += piece_values[WHITE_BISHOPS_INDEX];
      }
      else if(flags == ROOK_PROMO || flags == ROOK_PROMO_CAPTURE) {
        score += piece_values[WHITE_ROOKS_INDEX];
      }
      else {
        score += piece_values[WHITE_QUEENS_INDEX];
      }
    }
    /* check recapturing moves */
    if(to == recapture_square) {
      score += 5 * abs(piece_values[INDEX_FROM_PIECE(mv_piece)]); // arbitrary multiplication
    }
    else if(IS_CAPTURE(mv)) {
      // score += see_capture(mv); /* this function isn't fast enough I need incrementally updated attack tables */
      tar_piece = b.sq_board[to];
      if(!is_attacked((square)to, b.all_pieces)) {
        score += 5 * abs(piece_values[INDEX_FROM_PIECE(tar_piece)]);
      }
      else {
        score += abs(piece_values[INDEX_FROM_PIECE(tar_piece)]) - abs(piece_values[INDEX_FROM_PIECE(mv_piece)]);    
      }
    }
    /* score moves to squares attacked by pawns */
    else if(PIECE(mv_piece) != PAWN && is_attacked_by_pawn((square)to)) 
      score -= abs(piece_values[INDEX_FROM_PIECE(mv_piece)]); // can play around with this
    
    // done for better endgame move ordering of king moves
    if(PIECE(mv_piece) == KING && b.piece_boards[WHITE_QUEENS_INDEX] == 0 && b.piece_boards[BLACK_QUEENS_INDEX] == 0){
      score += perspective * (piece_scores[INDEX_FROM_PIECE(mv_piece) + 2][to] - piece_scores[INDEX_FROM_PIECE(mv_piece) + 2][from]);
    }
    else {
      score += perspective * (piece_scores[INDEX_FROM_PIECE(mv_piece)][to] - piece_scores[INDEX_FROM_PIECE(mv_piece)][from]);
    }

    // if(flags == QUIET_MOVE) {
    //     score -= 1000; /* try quiet moves last even behind bad captures */
    // }
        
    moves[i] = ADD_SCORE_TO_MOVE(mv, (signed int)score); // convert to signed int to sign extend to 32 bits
  }
  std::sort(moves.begin(), moves.end(), std::greater<move_t>());
  return;
}

void make_move(move_t move) {
  /* make a copy of the irreversible aspects of the position */
  state_t prev_state = b.state_history.top();
  state_t state = prev_state;
  b.ply++; /* we do this here to be able to update irr_ply */

  hash_val board_hash = b.board_hash;
  hash_val piece_hash = b.piece_hash;
  hash_val pawn_hash = b.pawn_hash;

  int from = FROM(move);
  int to = TO(move);
  int flags = FLAGS(move);

  /* 
    always have to remove the piece from its square...
    if promotion, you cannot place the same piece on to square
   */
  piece moving_piece = b.sq_board[from];
  bitboard *moving_piece_board = &b.piece_boards[INDEX_FROM_PIECE(moving_piece)];
  REM_PIECE(*moving_piece_board, from);
  b.sq_board[from] = EMPTY;

  
  if(PIECE(moving_piece) != KING) // king done seperately during eval for endgame
    b.positional_score -= piece_scores[INDEX_FROM_PIECE(moving_piece)][from];
  
  /* XOR out the piece from hash value */
  hash_val from_zobrist = zobrist_table.table[from][INDEX_FROM_PIECE(moving_piece)];
  board_hash ^= from_zobrist;
  piece_hash ^= from_zobrist;
  if(PIECE(moving_piece) == PAWN) {
    pawn_hash ^= from_zobrist;
  }
  
  /* update the king locations and castling rights */
  if(moving_piece == (WHITE | KING)) {
    b.white_king_loc = (square)to;
    REM_WHITE_KING_SIDE(state);
    REM_WHITE_QUEEN_SIDE(state);
  }
  else if(moving_piece == (BLACK | KING)) {
    b.black_king_loc = (square)to;
    REM_BLACK_KING_SIDE(state);
    REM_BLACK_QUEEN_SIDE(state);
  }
  else if(moving_piece == (WHITE | ROOK) && from == H1) {
    REM_WHITE_KING_SIDE(state);
  }
  else if(moving_piece == (WHITE | ROOK) && from == A1) {
    REM_WHITE_QUEEN_SIDE(state);
  }
  else if(moving_piece == (BLACK | ROOK) && from == H8) {
    REM_BLACK_KING_SIDE(state);
  }
  else if(moving_piece == (BLACK | ROOK) && from == A8) {
    REM_BLACK_QUEEN_SIDE(state);
  }

  /* default there to be no en passant square and set it if double pawn push */
  SET_EN_PASSANT_SQ(state, NONE);

  bitboard *rook_board;
  piece captured_piece = EMPTY;
  bitboard *captured_board;
  piece promo_piece = EMPTY;
  bitboard *promo_board;
  int opponent_pawn_sq;
  // std::std::cout << to << std::endl;
  hash_val to_zobrist = zobrist_table.table[to][INDEX_FROM_PIECE(moving_piece)];
  switch (flags) {
    case QUIET_MOVE:
      PLACE_PIECE(*moving_piece_board, to); // place the moving piece
      b.sq_board[to] = moving_piece;
      board_hash ^= to_zobrist; // place moving piece in hash value
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) 
        pawn_hash ^= to_zobrist;
      break;
    case DOUBLE_PUSH:
      PLACE_PIECE(*moving_piece_board, to); // place the moving piece
      b.sq_board[to] = moving_piece;
      board_hash ^= to_zobrist; // place the pawn in hash value
      piece_hash ^= to_zobrist;
      pawn_hash ^= to_zobrist;
      /* 
        if it's a double pawn push and we are starting on rank 2, its white pushing
        otherwise it is black pushing the pawn
      */
      if(RANK(from) == RANK_2)
        SET_EN_PASSANT_SQ(state, (from + 8));
      else
        SET_EN_PASSANT_SQ(state, (from - 8));
      // hash value update for en passant square happens later
      break;
    case KING_SIDE_CASTLE:
      PLACE_PIECE(*moving_piece_board, to); // place the moving piece
      b.sq_board[to] = moving_piece;
      board_hash ^= to_zobrist; // place the king in hash value
      piece_hash ^= to_zobrist;
      if(from == E1) { // white king side
        rook_board = &b.piece_boards[WHITE_ROOKS_INDEX];
        REM_PIECE(*rook_board, H1);
        PLACE_PIECE(*rook_board, F1);
        b.sq_board[H1] = EMPTY;
        b.sq_board[F1] = WHITE | ROOK;
        board_hash ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // remove white rook from H1
        board_hash ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // place white rook on F1
        piece_hash ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // remove white rook from H1
        piece_hash ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // place white rook on F1
        b.positional_score -= piece_scores[WHITE_ROOKS_INDEX][H1];
        b.positional_score += piece_scores[WHITE_ROOKS_INDEX][F1];
      }
      else { // black king side
        rook_board = &b.piece_boards[BLACK_ROOKS_INDEX];
        REM_PIECE(*rook_board, H8);
        PLACE_PIECE(*rook_board, F8);
        b.sq_board[H8] = EMPTY;
        b.sq_board[F8] = BLACK | ROOK;
        board_hash ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // remove black rook from H8
        board_hash ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // place black rook on F8
        piece_hash ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // remove black rook from H8
        piece_hash ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // place black rook on F8
        b.positional_score -= piece_scores[BLACK_ROOKS_INDEX][H8];
        b.positional_score += piece_scores[BLACK_ROOKS_INDEX][F8];
      }
      break;
    case QUEEN_SIDE_CASTLE:
      PLACE_PIECE(*moving_piece_board, to); // place the moving piece
      b.sq_board[to] = moving_piece;
      board_hash ^= to_zobrist; // place the king in hash value
      // piece_hash ^= to_zobrist;
      if(from == E1) { // white queen side
        rook_board = &b.piece_boards[WHITE_ROOKS_INDEX];
        REM_PIECE(*rook_board, A1);
        PLACE_PIECE(*rook_board, D1);
        b.sq_board[A1] = EMPTY;
        b.sq_board[D1] = WHITE | ROOK;
        board_hash ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // remove white rook from A1
        board_hash ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // place white rook on D1
        piece_hash ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // remove white rook from A1
        piece_hash ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // place white rook on D1
        b.positional_score -= piece_scores[WHITE_ROOKS_INDEX][A1];
        b.positional_score += piece_scores[WHITE_ROOKS_INDEX][D1];
      }
      else { // black queen side
        rook_board = &b.piece_boards[BLACK_ROOKS_INDEX];
        REM_PIECE(*rook_board, A8);
        PLACE_PIECE(*rook_board, D8);
        b.sq_board[A8] = EMPTY;
        b.sq_board[D8] = BLACK | ROOK;
        board_hash ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // remove black rook from A8
        board_hash ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // place black rook on D8
        piece_hash ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // remove black rook from A8
        piece_hash ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // place black rook on D8
        b.positional_score -= piece_scores[BLACK_ROOKS_INDEX][A8];
        b.positional_score += piece_scores[BLACK_ROOKS_INDEX][D8];
      }
      break;
    case NORMAL_CAPTURE:
      PLACE_PIECE(*moving_piece_board, to); // place the moving piece
      board_hash ^= to_zobrist; // place the moving piece in hash value
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) 
        pawn_hash ^= to_zobrist;

      /* remove the captured piece from it's bitboard */
      captured_piece = b.sq_board[to];
      captured_board = &b.piece_boards[INDEX_FROM_PIECE(captured_piece)];
      REM_PIECE(*captured_board, to);

      b.sq_board[to] = moving_piece;

      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)];
      if(PIECE(captured_piece) == PAWN)
        pawn_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)];

      /* remove castling rights if rook is captured in corner */
      if(PIECE(captured_piece) == ROOK) {
        if(to == H1) REM_WHITE_KING_SIDE(state);
        else if(to == A1) REM_WHITE_QUEEN_SIDE(state);
        else if(to == H8) REM_BLACK_KING_SIDE(state);
        else if(to == A8) REM_BLACK_QUEEN_SIDE(state);
      }
      break;
    case EN_PASSANT_CAPTURE:
      PLACE_PIECE(*moving_piece_board, to); // place the moving piece
      b.sq_board[to] = moving_piece;
      board_hash ^= to_zobrist; // place the pawn in hash value
      piece_hash ^= to_zobrist;
      pawn_hash ^= to_zobrist;

      /* distinguish between white and black en passant */
      opponent_pawn_sq = (RANK(to) == RANK_6) ? (to - 8) : (to + 8);

      /* remove the captured pawn */
      captured_piece = b.sq_board[opponent_pawn_sq];
      captured_board = &b.piece_boards[INDEX_FROM_PIECE(captured_piece)];
      REM_PIECE(*captured_board, opponent_pawn_sq);
      b.sq_board[opponent_pawn_sq] = EMPTY;
      board_hash ^= zobrist_table.table[opponent_pawn_sq][INDEX_FROM_PIECE(captured_piece)]; // remove the captured pawn from hash value
      piece_hash ^= zobrist_table.table[opponent_pawn_sq][INDEX_FROM_PIECE(captured_piece)];
      pawn_hash ^= zobrist_table.table[opponent_pawn_sq][INDEX_FROM_PIECE(captured_piece)];
      break;
    case KNIGHT_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = b.sq_board[to];
      captured_board = &b.piece_boards[INDEX_FROM_PIECE(captured_piece)];
      REM_PIECE(*captured_board, to);
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)];
      /* fallthrough */
    case KNIGHT_PROMO:
      if(b.t == W) {promo_piece = WHITE | KNIGHT; promo_board = &b.piece_boards[WHITE_KNIGHTS_INDEX];}
      else         {promo_piece = BLACK | KNIGHT; promo_board = &b.piece_boards[BLACK_KNIGHTS_INDEX];}
      PLACE_PIECE(*promo_board, to);
      b.sq_board[to] = promo_piece;
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place knight in hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      break;
    case BISHOP_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = b.sq_board[to];
      captured_board = &b.piece_boards[INDEX_FROM_PIECE(captured_piece)];
      REM_PIECE(*captured_board, to);
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)];
      /* fallthrough */
    case BISHOP_PROMO:
      if(b.t == W) {promo_piece = WHITE | BISHOP; promo_board = &b.piece_boards[WHITE_BISHOPS_INDEX];}
      else         {promo_piece = BLACK | BISHOP; promo_board = &b.piece_boards[BLACK_BISHOPS_INDEX];}
      PLACE_PIECE(*promo_board, to);
      b.sq_board[to] = promo_piece;
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place bishop in hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      break;
    case ROOK_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = b.sq_board[to];
      captured_board = &b.piece_boards[INDEX_FROM_PIECE(captured_piece)];
      REM_PIECE(*captured_board, to);
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)];
      /* fallthrough */
    case ROOK_PROMO:
      if(b.t == W) {promo_piece = WHITE | ROOK; promo_board = &b.piece_boards[WHITE_ROOKS_INDEX];}
      else         {promo_piece = BLACK | ROOK; promo_board = &b.piece_boards[BLACK_ROOKS_INDEX];}
      PLACE_PIECE(*promo_board, to);
      b.sq_board[to] = promo_piece;
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place rook in hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      break;
    case QUEEN_PROMO_CAPTURE:
      /* remove the captured piece from it's bitboard */
      captured_piece = b.sq_board[to];
      captured_board = &b.piece_boards[INDEX_FROM_PIECE(captured_piece)];
      REM_PIECE(*captured_board, to);
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)]; // remove the captured piece from hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(captured_piece)];
      /* fallthrough */
    case QUEEN_PROMO:
      if(b.t == W) {promo_piece = WHITE | QUEEN; promo_board = &b.piece_boards[WHITE_QUEENS_INDEX];}
      else         {promo_piece = BLACK | QUEEN; promo_board = &b.piece_boards[BLACK_QUEENS_INDEX];}
      PLACE_PIECE(*promo_board, to);
      b.sq_board[to] = promo_piece;
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)]; // place queen in hash value
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      break;
  }

  /* update hash value castling rights */
  if(b.t == W) { // castling rights have changed
    if(WHITE_KING_SIDE(prev_state) && !WHITE_KING_SIDE(state))
      board_hash ^= zobrist_table.white_king_side;
    if(WHITE_QUEEN_SIDE(prev_state) && !WHITE_QUEEN_SIDE(state))
      board_hash ^= zobrist_table.white_queen_side;
  }
  else { 
    if(BLACK_KING_SIDE(prev_state) && !BLACK_KING_SIDE(state))
      board_hash ^= zobrist_table.black_king_side;
    if(BLACK_QUEEN_SIDE(prev_state) && !BLACK_QUEEN_SIDE(state))
      board_hash ^= zobrist_table.black_queen_side;
  }

  /* update en passant file in hash value */
  square prev_en_passant_sq = (square)EN_PASSANT_SQ(prev_state);
  square curr_en_passant_sq = (square)EN_PASSANT_SQ(state);
  if(prev_en_passant_sq != NONE)
    board_hash ^= zobrist_table.en_passant_file[FILE(prev_en_passant_sq)]; // remove the last board's en passant from hash value
  
  if(curr_en_passant_sq != NONE)
    board_hash ^= zobrist_table.en_passant_file[FILE(curr_en_passant_sq)]; // place the current en passant file in hash value

  /* add the last captured piece to the state */
  SET_LAST_CAPTURE(state, captured_piece);

  /* update evaluation items */
  if(captured_piece != EMPTY){
    if(flags == EN_PASSANT_CAPTURE)
      b.positional_score -= piece_scores[INDEX_FROM_PIECE(captured_piece)][opponent_pawn_sq];
    else
      b.positional_score -= piece_scores[INDEX_FROM_PIECE(captured_piece)][to];
    b.material_score -= piece_values[INDEX_FROM_PIECE(captured_piece)];
    b.piece_counts[INDEX_FROM_PIECE(captured_piece)]--;
    b.total_material -= abs(piece_values[INDEX_FROM_PIECE(captured_piece)]);
  }

  if(promo_piece != EMPTY) {
    if(COLOR(promo_piece) == WHITE) {
      b.material_score -= piece_values[WHITE_PAWNS_INDEX];
      b.piece_counts[WHITE_PAWNS_INDEX]--;
      b.total_material -= abs(piece_values[WHITE_PAWNS_INDEX]);
    }
    else {
      b.material_score -= piece_values[BLACK_PAWNS_INDEX];
      b.piece_counts[BLACK_PAWNS_INDEX]--;
      b.total_material -= abs(piece_values[BLACK_PAWNS_INDEX]);
    }
    b.material_score += piece_values[INDEX_FROM_PIECE(promo_piece)];
    b.positional_score += piece_scores[INDEX_FROM_PIECE(promo_piece)][to];
    b.piece_counts[INDEX_FROM_PIECE(promo_piece)]++;
    b.total_material += abs(piece_values[INDEX_FROM_PIECE(promo_piece)]);
  }
  else if(PIECE(moving_piece) != KING) {
    b.positional_score += piece_scores[INDEX_FROM_PIECE(moving_piece)][to];
  }

  /* if we make an irreversible move, remember it! */
  if(IS_CAPTURE(move) || IS_PROMO(move) || PIECE(moving_piece) == PAWN)
    SET_IRR_PLY(state, b.ply);
  
  b.t = !b.t;
  /* reverse the black_to_move hash */
  board_hash ^= zobrist_table.black_to_move;

  update_boards();
  SET_LAST_MOVE(state, move);
  b.board_hash = board_hash;
  b.piece_hash = piece_hash;
  b.pawn_hash = pawn_hash;
  b.state_history.push(state);
  b.board_history.push_back(board_hash);
  // game_history.insert(board_hash); /* insert the new board hash into the game history */
  return;
}

void unmake_move(move_t move) {
  /* make a copy of the irreversible aspects of the position */
  state_t state = b.state_history.top();
  b.ply--;
  b.board_history.pop_back();

  hash_val board_hash = b.board_hash;
  hash_val piece_hash = b.piece_hash;
  hash_val pawn_hash = b.pawn_hash;

  // game_history.erase(board_hash); /* remove the board hash from the game history */

  int from = FROM(move);
  int to = TO(move);
  int flags = FLAGS(move);

  piece moving_piece = b.sq_board[to];
  bitboard *moving_piece_board;
  bitboard *rook_board;
  piece captured_piece = EMPTY;
  bitboard *captured_board;
  piece promo_piece = EMPTY;
  bitboard *promo_board;
  int opponent_pawn_sq;
  int mv_index = INDEX_FROM_PIECE(moving_piece);
  int cap_index;
  hash_val from_zobrist = zobrist_table.table[from][mv_index];
  hash_val to_zobrist = zobrist_table.table[to][mv_index];
  switch (flags) {
    case QUIET_MOVE:
      /* the moving piece will always be the same piece unless we are dealing with a promotion */
      moving_piece = b.sq_board[to];
      moving_piece_board = &b.piece_boards[mv_index];
      PLACE_PIECE(*moving_piece_board, from);
      REM_PIECE(*moving_piece_board, to);
      b.sq_board[from] = moving_piece;
      b.sq_board[to] = EMPTY;

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) {
        pawn_hash ^= from_zobrist;
        pawn_hash ^= to_zobrist;
      }
      break;
    case DOUBLE_PUSH:
      moving_piece = b.sq_board[to];
      mv_index = INDEX_FROM_PIECE(moving_piece);
      moving_piece_board = &b.piece_boards[mv_index];
      PLACE_PIECE(*moving_piece_board, from);
      REM_PIECE(*moving_piece_board, to);
      b.sq_board[from] = moving_piece;
      b.sq_board[to] = EMPTY;

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      pawn_hash ^= from_zobrist;
      pawn_hash ^= to_zobrist;
      break;
    case KING_SIDE_CASTLE:
      moving_piece = b.sq_board[to];
      mv_index = INDEX_FROM_PIECE(moving_piece);
      moving_piece_board = &b.piece_boards[mv_index];
      PLACE_PIECE(*moving_piece_board, from); /* place the king back on its uncastled square */
      REM_PIECE(*moving_piece_board, to); /* remove the king from its castled location */
      b.sq_board[from] = moving_piece;
      b.sq_board[to] = EMPTY;
      
      board_hash ^= from_zobrist; /* remove the castled king from hash value */
      board_hash ^= to_zobrist; /* place the uncastled king in hash value */
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;

      if(from == E1) { // white king side
        rook_board = &b.piece_boards[WHITE_ROOKS_INDEX];
        REM_PIECE(*rook_board, F1);
        PLACE_PIECE(*rook_board, H1);
        b.sq_board[F1] = EMPTY;
        b.sq_board[H1] = WHITE | ROOK;
        board_hash ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // remove white rook from F1
        board_hash ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // place white rook on H1
        piece_hash ^= zobrist_table.table[F1][WHITE_ROOKS_INDEX]; // remove white rook from F1
        piece_hash ^= zobrist_table.table[H1][WHITE_ROOKS_INDEX]; // place white rook on H1
        b.positional_score -= piece_scores[WHITE_ROOKS_INDEX][F1];
        b.positional_score += piece_scores[WHITE_ROOKS_INDEX][H1];
      }
      else { // black king side
        rook_board = &b.piece_boards[BLACK_ROOKS_INDEX];
        REM_PIECE(*rook_board, F8);
        PLACE_PIECE(*rook_board, H8);
        b.sq_board[F8] = EMPTY;
        b.sq_board[H8] = BLACK | ROOK;
        board_hash ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // remove black rook from F8
        board_hash ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // place black rook on H8
        piece_hash ^= zobrist_table.table[F8][BLACK_ROOKS_INDEX]; // remove black rook from F8
        piece_hash ^= zobrist_table.table[H8][BLACK_ROOKS_INDEX]; // place black rook on H8
        b.positional_score -= piece_scores[BLACK_ROOKS_INDEX][F8];
        b.positional_score += piece_scores[BLACK_ROOKS_INDEX][H8];
      }
      break;
    case QUEEN_SIDE_CASTLE:
      moving_piece = b.sq_board[to];
      mv_index = INDEX_FROM_PIECE(moving_piece);
      moving_piece_board = &b.piece_boards[mv_index];
      PLACE_PIECE(*moving_piece_board, from); /* place the king back on its uncastled square */
      REM_PIECE(*moving_piece_board, to); /* remove the king from its castled location */
      b.sq_board[from] = moving_piece;
      b.sq_board[to] = EMPTY;
      
      board_hash ^= from_zobrist; /* remove the castled king from hash value */
      board_hash ^= to_zobrist; /* place the uncastled king in hash value */
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;

      if(from == E1) { // white queen side
        rook_board = &b.piece_boards[WHITE_ROOKS_INDEX];
        REM_PIECE(*rook_board, D1);
        PLACE_PIECE(*rook_board, A1);
        b.sq_board[D1] = EMPTY;
        b.sq_board[A1] = WHITE | ROOK;
        board_hash ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // remove white rook from D1
        board_hash ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // place white rook on A1
        piece_hash ^= zobrist_table.table[D1][WHITE_ROOKS_INDEX]; // remove white rook from D1
        piece_hash ^= zobrist_table.table[A1][WHITE_ROOKS_INDEX]; // place white rook on A1
        b.positional_score -= piece_scores[WHITE_ROOKS_INDEX][D1];
        b.positional_score += piece_scores[WHITE_ROOKS_INDEX][A1];
      }
      else { // black queen side
        rook_board = &b.piece_boards[BLACK_ROOKS_INDEX];
        REM_PIECE(*rook_board, D8);
        PLACE_PIECE(*rook_board, A8);
        b.sq_board[D8] = EMPTY;
        b.sq_board[A8] = BLACK | ROOK;
        board_hash ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // remove black rook from D8
        board_hash ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // place black rook on A8
        piece_hash ^= zobrist_table.table[D8][BLACK_ROOKS_INDEX]; // remove black rook from D8
        piece_hash ^= zobrist_table.table[A8][BLACK_ROOKS_INDEX]; // place black rook on A8
        b.positional_score -= piece_scores[BLACK_ROOKS_INDEX][D8];
        b.positional_score += piece_scores[BLACK_ROOKS_INDEX][A8];
      }
      break;
    case NORMAL_CAPTURE:
      moving_piece = b.sq_board[to];
      mv_index = INDEX_FROM_PIECE(moving_piece);
      moving_piece_board = &b.piece_boards[mv_index];
      PLACE_PIECE(*moving_piece_board, from);
      REM_PIECE(*moving_piece_board, to);
      b.sq_board[from] = moving_piece;

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      if(PIECE(moving_piece) == PAWN) {
        pawn_hash ^= from_zobrist;
        pawn_hash ^= to_zobrist;
      }

      /* place the captured piece back where it was captured from */
      captured_piece = LAST_CAPTURE(state);
      cap_index = INDEX_FROM_PIECE(captured_piece);
      captured_board = &b.piece_boards[cap_index];
      PLACE_PIECE(*captured_board, to);
      b.sq_board[to] = captured_piece;

      board_hash ^= zobrist_table.table[to][cap_index]; /* place the captured piece in the hash value */
      piece_hash ^= zobrist_table.table[to][cap_index];
      if(PIECE(captured_piece) == PAWN)
        pawn_hash ^= zobrist_table.table[to][cap_index];
      break;
    case EN_PASSANT_CAPTURE:
      moving_piece = b.sq_board[to];
      mv_index = INDEX_FROM_PIECE(moving_piece);
      moving_piece_board = &b.piece_boards[mv_index];
      PLACE_PIECE(*moving_piece_board, from); 
      REM_PIECE(*moving_piece_board, to);
      b.sq_board[from] = moving_piece;
      b.sq_board[to] = EMPTY;

      board_hash ^= from_zobrist;
      board_hash ^= to_zobrist;
      piece_hash ^= from_zobrist;
      piece_hash ^= to_zobrist;
      pawn_hash ^= from_zobrist;
      pawn_hash ^= to_zobrist;

      /* distinguish between white and black en passant */
      opponent_pawn_sq = (RANK(to) == RANK_6) ? (to - 8) : (to + 8);

      /* place the captured pawn */
      captured_piece = LAST_CAPTURE(state);
      cap_index = INDEX_FROM_PIECE(captured_piece);
      captured_board = &b.piece_boards[cap_index];
      PLACE_PIECE(*captured_board, opponent_pawn_sq);
      b.sq_board[opponent_pawn_sq] = captured_piece;
      board_hash ^= zobrist_table.table[opponent_pawn_sq][cap_index]; // place the captured pawn in hash value
      piece_hash ^= zobrist_table.table[opponent_pawn_sq][cap_index];
      pawn_hash ^= zobrist_table.table[opponent_pawn_sq][cap_index];
      break;
    case KNIGHT_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = LAST_CAPTURE(state);
      cap_index = INDEX_FROM_PIECE(captured_piece);
      captured_board = &b.piece_boards[cap_index];
      PLACE_PIECE(*captured_board, to);
      b.sq_board[to] = captured_piece;
      board_hash ^= zobrist_table.table[to][cap_index]; /* place captured piece back in hash value */
      piece_hash ^= zobrist_table.table[to][cap_index];
      /* fallthrough */
    case KNIGHT_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(b.t == B) {
        moving_piece = WHITE | PAWN; 
        moving_piece_board = &b.piece_boards[WHITE_PAWNS_INDEX]; 
        promo_piece = WHITE | KNIGHT; 
        promo_board = &b.piece_boards[WHITE_KNIGHTS_INDEX];
      }
      else {
        moving_piece = BLACK | PAWN; 
        moving_piece_board = &b.piece_boards[BLACK_PAWNS_INDEX];
        promo_piece = BLACK | KNIGHT; 
        promo_board = &b.piece_boards[BLACK_KNIGHTS_INDEX];
      }
      mv_index = INDEX_FROM_PIECE(moving_piece);
      PLACE_PIECE(*moving_piece_board, from); /* place the pawn back */
      b.sq_board[from] = moving_piece;
      board_hash ^= zobrist_table.table[from][mv_index];
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      piece_hash ^= zobrist_table.table[from][mv_index];
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      pawn_hash ^= zobrist_table.table[from][mv_index];

      REM_PIECE(*promo_board, to);
      if(!IS_CAPTURE(move)) /* if they aren't capturing anything, then make the promotion square empty */
        b.sq_board[to] = EMPTY;
      break;
    case BISHOP_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = LAST_CAPTURE(state);
      cap_index = INDEX_FROM_PIECE(captured_piece);
      captured_board = &b.piece_boards[cap_index];
      PLACE_PIECE(*captured_board, to);
      b.sq_board[to] = captured_piece;
      board_hash ^= zobrist_table.table[to][cap_index]; /* place captured piece back in hash value */
      piece_hash ^= zobrist_table.table[to][cap_index];
      /* fallthrough */
    case BISHOP_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(b.t == B) {
        moving_piece = WHITE | PAWN; 
        moving_piece_board = &b.piece_boards[WHITE_PAWNS_INDEX]; 
        promo_piece = WHITE | BISHOP; 
        promo_board = &b.piece_boards[WHITE_BISHOPS_INDEX];
      }
      else {
        moving_piece = BLACK | PAWN; 
        moving_piece_board = &b.piece_boards[BLACK_PAWNS_INDEX];
        promo_piece = BLACK | BISHOP; 
        promo_board = &b.piece_boards[BLACK_BISHOPS_INDEX];
      }
      mv_index = INDEX_FROM_PIECE(moving_piece);
      PLACE_PIECE(*moving_piece_board, from); /* place the pawn back */
      b.sq_board[from] = moving_piece;
      board_hash ^= zobrist_table.table[from][mv_index];
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      piece_hash ^= zobrist_table.table[from][mv_index];
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      pawn_hash ^= zobrist_table.table[from][mv_index];

      REM_PIECE(*promo_board, to);
      if(!IS_CAPTURE(move)) /* if they aren't capturing anything, then make the promotion square empty */
        b.sq_board[to] = EMPTY;
      break;
    case ROOK_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = LAST_CAPTURE(state);
      cap_index = INDEX_FROM_PIECE(captured_piece);
      captured_board = &b.piece_boards[cap_index];
      PLACE_PIECE(*captured_board, to);
      b.sq_board[to] = captured_piece;
      board_hash ^= zobrist_table.table[to][cap_index]; /* place captured piece back in hash value */
      piece_hash ^= zobrist_table.table[to][cap_index];
      /* fallthrough */
    case ROOK_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(b.t == B) {
        moving_piece = WHITE | PAWN; 
        moving_piece_board = &b.piece_boards[WHITE_PAWNS_INDEX]; 
        promo_piece = WHITE | ROOK; 
        promo_board = &b.piece_boards[WHITE_ROOKS_INDEX];
      }
      else {
        moving_piece = BLACK | PAWN; 
        moving_piece_board = &b.piece_boards[BLACK_PAWNS_INDEX];
        promo_piece = BLACK | ROOK; 
        promo_board = &b.piece_boards[BLACK_ROOKS_INDEX];
      }
      mv_index = INDEX_FROM_PIECE(moving_piece);
      PLACE_PIECE(*moving_piece_board, from); /* place the pawn back */
      b.sq_board[from] = moving_piece;
      board_hash ^= zobrist_table.table[from][mv_index];
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      piece_hash ^= zobrist_table.table[from][mv_index];
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      pawn_hash ^= zobrist_table.table[from][mv_index];

      REM_PIECE(*promo_board, to);
      if(!IS_CAPTURE(move)) /* if they aren't capturing anything, then make the promotion square empty */
        b.sq_board[to] = EMPTY;
      break;
    case QUEEN_PROMO_CAPTURE:
      /* place the captured piece back */
      captured_piece = LAST_CAPTURE(state);
      cap_index = INDEX_FROM_PIECE(captured_piece);
      captured_board = &b.piece_boards[cap_index];
      PLACE_PIECE(*captured_board, to);
      b.sq_board[to] = captured_piece;
      board_hash ^= zobrist_table.table[to][cap_index]; /* place captured piece back in hash value */
      piece_hash ^= zobrist_table.table[to][cap_index];
      /* fallthrough */
    case QUEEN_PROMO:
      /* if it is currently black's turn, then white must have been the one to promote */
      if(b.t == B) {
        moving_piece = WHITE | PAWN; 
        moving_piece_board = &b.piece_boards[WHITE_PAWNS_INDEX]; 
        promo_piece = WHITE | QUEEN; 
        promo_board = &b.piece_boards[WHITE_QUEENS_INDEX];
      }
      else {
        moving_piece = BLACK | PAWN; 
        moving_piece_board = &b.piece_boards[BLACK_PAWNS_INDEX];
        promo_piece = BLACK | QUEEN; 
        promo_board = &b.piece_boards[BLACK_QUEENS_INDEX];
      }
      mv_index = INDEX_FROM_PIECE(moving_piece);
      PLACE_PIECE(*moving_piece_board, from); /* place the pawn back */
      b.sq_board[from] = moving_piece;
      board_hash ^= zobrist_table.table[from][mv_index];
      board_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      piece_hash ^= zobrist_table.table[from][mv_index];
      piece_hash ^= zobrist_table.table[to][INDEX_FROM_PIECE(promo_piece)];
      pawn_hash ^= zobrist_table.table[from][mv_index];

      REM_PIECE(*promo_board, to);
      if(!IS_CAPTURE(move)) /* if they aren't capturing anything, then make the promotion square empty */
        b.sq_board[to] = EMPTY;
      break;
  }

  if(moving_piece == (WHITE | KING))
    b.white_king_loc = (square)from;
  else if(moving_piece == (BLACK | KING))
    b.black_king_loc = (square)from;

  board_hash ^= zobrist_table.black_to_move;

  b.state_history.pop(); // go back to previous board's irreversible state
  state_t prev_state = b.state_history.top();

  /* update hash value castling rights */
  if(b.t == B) { // castling rights have changed
    if(WHITE_KING_SIDE(prev_state) && !WHITE_KING_SIDE(state))
      board_hash ^= zobrist_table.white_king_side;
    if(WHITE_QUEEN_SIDE(prev_state) && !WHITE_QUEEN_SIDE(state))
      board_hash ^= zobrist_table.white_queen_side;
  }
  else { 
    if(BLACK_KING_SIDE(prev_state) && !BLACK_KING_SIDE(state))
      board_hash ^= zobrist_table.black_king_side;
    if(BLACK_QUEEN_SIDE(prev_state) && !BLACK_QUEEN_SIDE(state))
      board_hash ^= zobrist_table.black_queen_side;
  }

  /* update en passant file in hash value */
  square prev_en_passant_sq = (square)EN_PASSANT_SQ(prev_state);
  square curr_en_passant_sq = (square)EN_PASSANT_SQ(state);
  if(prev_en_passant_sq != NONE)
    board_hash ^= zobrist_table.en_passant_file[FILE(prev_en_passant_sq)]; // remove the last board's en passant from hash value
  if(curr_en_passant_sq != NONE)
    board_hash ^= zobrist_table.en_passant_file[FILE(curr_en_passant_sq)]; // place the current en passant file in hash value

  /* update evaluation items */
  if(captured_piece != EMPTY){
    if(flags == EN_PASSANT_CAPTURE)
      b.positional_score += piece_scores[INDEX_FROM_PIECE(captured_piece)][opponent_pawn_sq];
    else
      b.positional_score += piece_scores[INDEX_FROM_PIECE(captured_piece)][to];
    b.material_score += piece_values[INDEX_FROM_PIECE(captured_piece)];
    b.piece_counts[INDEX_FROM_PIECE(captured_piece)]++;
    b.total_material += abs(piece_values[INDEX_FROM_PIECE(captured_piece)]);
  }

  if(promo_piece != EMPTY) {
    if(COLOR(promo_piece) == WHITE) {
      b.material_score += piece_values[WHITE_PAWNS_INDEX];
      b.piece_counts[WHITE_PAWNS_INDEX]++;
      b.positional_score += piece_scores[WHITE_PAWNS_INDEX][from];
      b.total_material += abs(piece_values[WHITE_PAWNS_INDEX]);
    }
    else {
      b.material_score += piece_values[BLACK_PAWNS_INDEX];
      b.piece_counts[BLACK_PAWNS_INDEX]++;
      b.positional_score += piece_scores[BLACK_PAWNS_INDEX][from];
      b.total_material += abs(piece_values[BLACK_PAWNS_INDEX]);
    }
    b.material_score -= piece_values[INDEX_FROM_PIECE(promo_piece)];
    b.positional_score -= piece_scores[INDEX_FROM_PIECE(promo_piece)][to];
    b.piece_counts[INDEX_FROM_PIECE(promo_piece)]--;
    b.total_material -= abs(piece_values[INDEX_FROM_PIECE(promo_piece)]);
  }
  else if(PIECE(moving_piece) != KING) {
    b.positional_score -= piece_scores[INDEX_FROM_PIECE(moving_piece)][to];
    b.positional_score += piece_scores[INDEX_FROM_PIECE(moving_piece)][from];
  }

  b.t = !b.t;
  b.board_hash = board_hash;
  b.piece_hash = piece_hash;
  b.pawn_hash = pawn_hash;
  update_boards();
  return;
}

void make_nullmove() {
  state_t prev_state = b.state_history.top();
  state_t state = prev_state;
  square prev_en_passant = (square)EN_PASSANT_SQ(prev_state);
  if(prev_en_passant != NONE)
    b.board_hash ^= zobrist_table.en_passant_file[FILE(prev_en_passant)];
  SET_EN_PASSANT_SQ(state, NONE);
  SET_LAST_MOVE(state, NO_MOVE);
  b.t = !b.t;
  b.board_hash ^= zobrist_table.black_to_move;
  b.state_history.push(state);
  return;
}

void unmake_nullmove() {
  b.state_history.pop();
  state_t state = b.state_history.top();
  square en_passant_sq = (square)EN_PASSANT_SQ(state);
  if(en_passant_sq != NONE)
    b.board_hash ^= zobrist_table.en_passant_file[FILE(en_passant_sq)];
  b.t = !b.t;
  b.board_hash ^= zobrist_table.black_to_move;
  return;
}

/*
 * Goes from a move struct to the correct notation, given a move, a list of 
 * legal moves in the position, and the state of the board.
 */
std::string notation_from_move(move_t move) {
  std::vector<move_t> all_moves;
  generate_moves(all_moves);
  // conflicting doesn't work for knights right now
  // need to update for check (+) and checkmate (#)
  // need to add castling
  std::vector<move_t> conflicting_moves;
  for (move_t single_move : all_moves) {
    if(TO(single_move) == TO(move) && 
       FROM(single_move) != FROM(move) && 
       b.sq_board[FROM(single_move)] == b.sq_board[FROM(move)] &&
       FLAGS(single_move) == FLAGS(move))
      conflicting_moves.push_back(single_move);
  }
  const std::string files = "abcdefgh";
  const std::string ranks = "12345678";
  const std::string pieces = "PNBRQK";
  std::string str_move;
  piece mv_piece = b.sq_board[FROM(move)];
  char piece_name = pieces[INDEX_FROM_PIECE(mv_piece) / 2];
  bool capture = IS_CAPTURE(move);
  bool promotion = IS_PROMO(move);
  size_t start_file_num = FILE(FROM(move));
  size_t start_rank_num = RANK(FROM(move));
  size_t tar_file_num = FILE(TO(move));
  size_t tar_rank_num = RANK(TO(move));
  char start_file = files[start_file_num];
  char start_rank = ranks[start_rank_num];
  char tar_file = files[tar_file_num];
  char tar_rank = ranks[tar_rank_num];
  bool file_conflict = false;
  bool rank_conflict = false;
  size_t conflict_file_num;
  size_t conflict_rank_num;
  square start = (square)FROM(move);
  square target = (square)TO(move);

  if(piece_name == 'P' && capture) {
    str_move.push_back(start_file);
  }
  else if (piece_name == 'K' && 
      (start == E1 && target == G1 || 
       start == E8 && target == G8)) {
    return "O-O"; // this won't quite work for adding check and checkmate
  }
  else if (piece_name == 'K' && 
      (start == E1 && target == C1 || 
       start == E8 && target == C8)) {
    return "O-O-O"; // this won't quite work for adding check and checkmate
  }
  else if(piece_name != 'P') {
    str_move.push_back(piece_name);
    for(move_t single_move : conflicting_moves) {
      conflict_file_num = FILE(FROM(single_move));
      conflict_rank_num = RANK(FROM(single_move));
      if(conflict_file_num == start_file_num) file_conflict = true;
      else if(conflict_rank_num == start_rank_num) rank_conflict = true;
    }
    if(file_conflict) str_move.push_back(start_rank);
    if(rank_conflict) str_move.push_back(start_file);
    if(!file_conflict && !rank_conflict && 
       conflicting_moves.size() > 0 && piece_name == 'N') {
      str_move.push_back(start_file);
    }
  }

  if(capture) str_move.push_back('x');
  str_move.push_back(tar_file);
  str_move.push_back(tar_rank);
  if(promotion) {
    str_move.push_back('=');
    int flags = FLAGS(move);
    char promo_piece_c;
    if(flags == KNIGHT_PROMO || flags == KNIGHT_PROMO_CAPTURE) promo_piece_c = 'N';
    else if(flags == BISHOP_PROMO || flags == BISHOP_PROMO_CAPTURE) promo_piece_c = 'B';
    else if(flags == ROOK_PROMO || flags == ROOK_PROMO_CAPTURE) promo_piece_c = 'R';
    else promo_piece_c = 'Q';
    str_move.push_back(promo_piece_c);
  }
  return str_move;
}

// this doesn't work for promotions yet, but that shouldn't be a problem in opening
/* make sure that this is called before the move is played */
move_t move_from_notation(std::string notation) {
  // std::cout << notation << endl;
  std::string notation_copy = notation;
  if(notation.length() == 0) {
    std::cout << "Empty notation!\n";
    int y;
    std::cin >> y;
    std::exit(-1);
  }
  notation.erase(remove(notation.begin(), notation.end(), '+'), notation.end());
  std::vector<move_t> moves;
  generate_moves(moves);
  // this is so ugly
  if(notation == "O-O") {
    for (move_t move : moves) {
      if(FLAGS(move) == KING_SIDE_CASTLE) return move;
    }
  }
  else if(notation == "O-O-O") {
    for (move_t move : moves) {
      if(FLAGS(move) == QUEEN_SIDE_CASTLE) return move;
    }
  }
  piece mv_piece;
  char c = notation[0];
  if(isupper(c)) {
    mv_piece = piece_from_move_char(c);
    notation = notation.substr(1, notation.length() - 1);
  }
  else mv_piece = PAWN;
  if(b.t == W) mv_piece |= WHITE;
  else mv_piece |= BLACK;

  std::string delimiter = "=";

  piece promotion_piece;
  size_t promotion_marker = notation.find(delimiter);
  if(promotion_marker == std::string::npos) { // didn't find = 
    promotion_piece = EMPTY;
  }
  else {
    promotion_piece = piece_from_move_char(notation.substr(promotion_marker + 1, notation.length() - promotion_marker)[0]);
    notation = notation.substr(0, promotion_marker);
  }

  if(b.t == W) {mv_piece |= WHITE; promotion_piece |= WHITE;}
  else {mv_piece |= BLACK; promotion_piece |= BLACK;}
  
  notation.erase(remove(notation.begin(), notation.end(), 'x'), notation.end());
  const std::string files = "abcdefgh";
  const std::string ranks = "12345678";

  int target_rank;
  int target_file;
  int start_rank = -1;
  int start_file = -1;
  square target_square;

  // std::cout << notation << endl;
  if(notation.length() == 2) { // no move conflict
    target_rank = ranks.find(notation[1]);
    target_file = files.find(notation[0]);
  }
  else if(notation.length() == 3) { // some conflict
    target_rank = ranks.find(notation[2]);
    target_file = files.find(notation[1]);

    if(isalpha(notation[0])) {
      start_file = files.find(notation[0]);
    }
    else {
      start_rank = ranks.find(notation[0]);
    }
  }
  else {
    target_rank = ranks.find(notation[3]);
    target_file = files.find(notation[2]);
    
    start_file = files.find(notation[0]);
    start_rank = ranks.find(notation[1]);
  }
  target_square = (square)(target_rank * 8 + target_file);
  // if(start_rank != -1) start_rank++;
  // if(start_file != -1) start_file++;
  // int x;
  // cin >> x;
  for (move_t move : moves) {
    if(TO(move) == target_square && b.sq_board[FROM(move)] == mv_piece) {
      if(start_rank == -1 && start_file == -1) return move;
      if(start_rank == -1 && start_file == FILE(FROM(move))) return move;
      if(start_rank == RANK(FROM(move)) && start_file == -1) return move;
      if(start_rank == RANK(FROM(move)) && start_file == FILE(FROM(move))) return move;
    }
  }
  std::cout << "Move: " << notation_copy << std::endl;
  print_squarewise(b.sq_board);
  int x;
  std::cout << "No match found!" << std::endl;
  std::cin >> x;
  std::exit(-1); // should match to a move
}

std::string algebraic_notation(move_t move) {
  std::string result = "";
  std::string files = "abcdefgh";
  std::string ranks = "12345678";
  
  int from_file = FILE(FROM(move));
  int from_rank = RANK(FROM(move));
  
  int to_file = FILE(TO(move));
  int to_rank = RANK(TO(move));

  result.push_back(files[from_file]);
  result.push_back(ranks[from_rank]);
  result.push_back(files[to_file]);
  result.push_back(ranks[to_rank]);
  return result;
}

void sort_by_algebraic_notation(std::vector<move_t> &moves) {
  sort(moves.begin(), moves.end(), 
      [](move_t a, move_t b) {
        return algebraic_notation(a) < algebraic_notation(b);
      } 
  );
}