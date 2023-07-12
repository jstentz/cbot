#include "include/move.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/attacks.h"
#include "include/pieces.h"
#include "include/evaluation.h"
#include "include/hashing.h"
#include "include/tt.h"
#include "include/move_gen.h"
#include "include/utils.h"

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

MoveGenerator::MoveGenerator(const Board& board) : m_board(board) {}

void MoveGenerator::generate_moves(std::vector<Move> &curr_moves, bool captures_only) const
{
  bitboard check_pieces = checking_pieces();
  bitboard capture_mask = 0xFFFFFFFFFFFFFFFF;
  bitboard push_mask = 0xFFFFFFFFFFFFFFFF;
  int friendly_king_loc = (m_board.is_white_turn()) ? m_board.get_white_king_loc() : m_board.get_black_king_loc();
  CheckType check = check_type(check_pieces);
  if (check == CheckType::DOUBLE) 
  {
    generate_king_moves(curr_moves, captures_only);
    return;
  }
  else if (check == CheckType::SINGLE) {
    capture_mask = check_pieces;
    int sq = first_set_bit(check_pieces);
    if (is_sliding_piece(m_board[sq])) 
    {
      push_mask = opponent_slider_rays_to_square(friendly_king_loc);
    }
    else 
    {
      push_mask = 0;
    }
  }
  bool pawn_check = (check_pieces & (m_board.get_piece_bitboard(WHITE | PAWN) | m_board.get_piece_bitboard(BLACK | PAWN))) != 0;
  bitboard check_mask = push_mask | capture_mask;
  Pin pin = get_pinned_pieces(friendly_king_loc); // maybe change this so that the board holds the pinned pieces info
  generate_pawn_moves(curr_moves, check_mask, pawn_check, pin, captures_only);
  generate_knight_moves(curr_moves, check_mask, pin, captures_only);
  generate_bishop_moves(curr_moves, check_mask, pin, captures_only);
  generate_rook_moves(curr_moves, check_mask, pin, captures_only);
  generate_queen_moves(curr_moves, check_mask, pin, captures_only);
  generate_king_moves(curr_moves, captures_only);
}

void MoveGenerator::order_moves(std::vector<Move>& moves, Move tt_best_move) const
{
  signed short int score;
  piece mv_piece;
  piece tar_piece;
  int to;
  int from;
  Move mv;
  int flags;
  int perspective = m_board.is_white_turn() ? 1 : -1;
  // maybe add a bonus for castling moves
  // add recapturing the piece that was last captured as a good bonus to check first
  // bigger bonus for the higher value piece being captured
  // just have the board store the move that was made to get to that position
  // still need to add the least_valued_attacker logic, not exactly sure how to implement
  Move last_move = m_board.get_last_move();
  int recapture_square = -1;
  if (!last_move.is_no_move() && last_move.is_capture()) 
  {
    recapture_square = last_move.to();
  }
  for (Move& mv : moves) 
  {
    score = 0;
    if (!tt_best_move.is_no_move() && mv == tt_best_move) 
    {
      score += 10000; // idk try the PV node first
    }
    to = mv.to();
    from = mv.from();
    flags = mv.type();
    mv_piece = m_board[from];
    if (mv.is_promo()) 
    {
      if (flags == Move::KNIGHT_PROMO || flags == Move::KNIGHT_PROMO_CAPTURE) {
        score += piece_values[constants::WHITE_KNIGHTS_INDEX]; // just use the white knights because positive value
      }
      else if (flags == Move::BISHOP_PROMO || flags == Move::BISHOP_PROMO_CAPTURE) {
        score += piece_values[constants::WHITE_BISHOPS_INDEX];
      }
      else if (flags == Move::ROOK_PROMO || flags == Move::ROOK_PROMO_CAPTURE) {
        score += piece_values[constants::WHITE_ROOKS_INDEX];
      }
      else {
        score += piece_values[constants::WHITE_QUEENS_INDEX];
      }
    }
    /* check recapturing moves */
    if (to == recapture_square) 
    {
      score += 5 * abs(piece_values[utils::index_from_pc(mv_piece)]); // arbitrary multiplication
    }
    else if (mv.is_capture()) {
      // score += see_capture(mv); /* this function isn't fast enough I need incrementally updated attack tables */
      tar_piece = m_board[to];
      if (!is_attacked(to, m_board.get_all_pieces())) 
      {
        score += 5 * abs(piece_values[utils::index_from_pc(tar_piece)]);
      }
      else 
      {
        score += abs(piece_values[utils::index_from_pc(tar_piece)]) - abs(piece_values[utils::index_from_pc(mv_piece)]);    
      }
    }
    /* score moves to squares attacked by pawns */
    else if(PIECE(mv_piece) != PAWN && is_attacked_by_pawn(to)) 
      score -= abs(piece_values[utils::index_from_pc(mv_piece)]); // can play around with this
    
    // done for better endgame move ordering of king moves
    if (PIECE(mv_piece) == KING && m_board.get_piece_bitboard(WHITE | QUEEN) == 0 && m_board.get_piece_bitboard(BLACK | QUEEN) == 0)
    {
      score += perspective * (piece_scores[utils::index_from_pc(mv_piece) + 2][to] - piece_scores[utils::index_from_pc(mv_piece) + 2][from]);
    }
    else 
    {
      score += perspective * (piece_scores[utils::index_from_pc(mv_piece)][to] - piece_scores[utils::index_from_pc(mv_piece)][from]);
    }

    // if(flags == QUIET_MOVE) {
    //     score -= 1000; /* try quiet moves last even behind bad captures */
    // }     
    mv.set_score(score);
  }
  std::sort(moves.begin(), moves.end(), [](Move mv1, Move mv2) { return mv1.score() > mv2.score(); });
}

std::string MoveGenerator::notation_from_move(Move move) const
{
  std::vector<Move> all_moves;
  generate_moves(all_moves);
  // conflicting doesn't work for knights right now
  // need to update for check (+) and checkmate (#)
  // need to add castling
  std::vector<Move> conflicting_moves;
  for (Move single_move : all_moves) 
  {
    if (single_move.to() == move.to() && 
       single_move.from() != move.from() && 
       m_board[single_move.from()] == m_board[move.from()] &&
       single_move.type() == move.type())
    {
      conflicting_moves.push_back(single_move);
    }
  }

  std::string str_move;
  piece mv_piece = m_board[move.from()];
  char piece_name = constants::PIECES[utils::index_from_pc(mv_piece) / 2];
  bool capture = move.is_capture();
  bool promotion = move.is_promo();
  size_t start_file_num = utils::file(move.from());
  size_t start_rank_num = utils::rank(move.from());
  size_t tar_file_num = utils::file(move.to());
  size_t tar_rank_num = utils::rank(move.to());
  char start_file = constants::FILES[start_file_num];
  char start_rank = constants::RANKS[start_rank_num];
  char tar_file = constants::FILES[tar_file_num];
  char tar_rank = constants::RANKS[tar_rank_num];
  bool file_conflict = false;
  bool rank_conflict = false;
  size_t conflict_file_num;
  size_t conflict_rank_num;
  int start = move.from();
  int target = move.to();

  if(piece_name == 'P' && capture) {
    str_move.push_back(start_file);
  }
  else if (piece_name == 'K' && 
      (start == constants::E1 && target == constants::G1 || 
       start == constants::E8 && target == constants::G8)) {
    return "O-O"; // this won't quite work for adding check and checkmate
  }
  else if (piece_name == 'K' && 
      (start == constants::E1 && target == constants::C1 || 
       start == constants::E8 && target == constants::C8)) {
    return "O-O-O"; // this won't quite work for adding check and checkmate
  }
  else if(piece_name != 'P') {
    str_move.push_back(piece_name);
    for(Move single_move : conflicting_moves) {
      conflict_file_num = utils::file(single_move.from());
      conflict_rank_num = utils::rank(single_move.from());
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
    int flags = move.type();
    char promo_piece_c;
    if(flags == Move::KNIGHT_PROMO || flags == Move::KNIGHT_PROMO_CAPTURE) promo_piece_c = 'N';
    else if(flags == Move::BISHOP_PROMO || flags == Move::BISHOP_PROMO_CAPTURE) promo_piece_c = 'B';
    else if(flags == Move::ROOK_PROMO || flags == Move::ROOK_PROMO_CAPTURE) promo_piece_c = 'R';
    else promo_piece_c = 'Q';
    str_move.push_back(promo_piece_c);
  }
  return str_move;
}

/// TODO: doesn't work for promotions, also this is so bad 
Move MoveGenerator::move_from_notation(std::string notation) const
{
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



// end new class structure 

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

int see(int sq) {
  int value = 0;
  square attacker_sq = least_valued_attacker_sq((square)sq, !b.t);
  if(attacker_sq != NONE) {
    move_t capture = construct_move(attacker_sq, sq, NORMAL_CAPTURE); /* would this also work for promotions? */
    piece captured_piece = b.sq_board[sq];
    make_move(capture);
    value = std::max(0, abs(piece_values[utils::index_from_pc(captured_piece)]) - see(sq));
    unmake_move(capture);
  }
  return value;
}

int see_capture(move_t capture) {
  int value = 0;
  int to = TO(capture);
  piece cap_piece = b.sq_board[to];
  make_move(capture);
  value = abs(piece_values[utils::index_from_pc(cap_piece)]) - see(to);
  unmake_move(capture);
  return value;
}

bool is_bad_capture(move_t capture) {
  piece mv_piece = b.sq_board[FROM(capture)];
  piece cap_piece = b.sq_board[TO(capture)];
  /* if we are capturing a piece of higher material, its probably good */
  if(abs(piece_values[utils::index_from_pc(cap_piece)]) 
     - abs(piece_values[utils::index_from_pc(mv_piece)]) > 50) {
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

move_t long_algebraic_to_move(std::string notation)
{
  const std::string files = "abcdefgh";
  const std::string ranks = "12345678";
  int from_file = files.find(notation[0]);
  int from_rank = ranks.find(notation[1]);
  int to_file = files.find(notation[2]);
  int to_rank = ranks.find(notation[3]);

  bool is_promo = false;
  int flags;
  int flags_capture;
  
  /* check for promotion */
  if (notation.size() > 4) 
  {
    is_promo = true;
    switch (notation[4])
    {
      case 'q':
        flags = QUEEN_PROMO;
        flags_capture = QUEEN_PROMO_CAPTURE;
        break;
      case 'b':
        flags = BISHOP_PROMO;
        flags_capture = BISHOP_PROMO_CAPTURE;
        break;
      case 'n':
        flags = KNIGHT_PROMO;
        flags_capture = KNIGHT_PROMO_CAPTURE;
        break;
      case 'r':
        flags = ROOK_PROMO;
        flags_capture = ROOK_PROMO_CAPTURE;
        break;
      default:
        exit(1);
    }
  }

  std::vector<move_t> moves;
  generate_moves(moves);

  for (move_t move : moves)
  {
    /* check for correct starting and ending square and promotion */
    if (
        (FILE(FROM(move)) == from_file) &&
        (RANK(FROM(move)) == from_rank) &&
        (FILE(TO(move)) == to_file) &&
        (RANK(TO(move)) == to_rank) &&
        (!is_promo || FLAGS(move) == flags || FLAGS(move) == flags_capture)
       )
    {
      return move;
    }
  }
  return 0;
}

std::string move_to_long_algebraic(move_t move) {
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
        return move_to_long_algebraic(a) < move_to_long_algebraic(b);
      } 
  );
}
