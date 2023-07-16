#include "include/move.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/attacks.h"
#include "include/pieces.h"
#include "include/move_gen.h"
#include "include/evaluation.h"
#include "include/utils.h"

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

MoveGenerator::MoveGenerator(Board::Ptr board) : m_board{board} {}

void MoveGenerator::generate_moves(std::vector<Move> &curr_moves, bool captures_only) const
{
  bitboard check_pieces = checking_pieces();
  bitboard capture_mask = 0xFFFFFFFFFFFFFFFF;
  bitboard push_mask = 0xFFFFFFFFFFFFFFFF;
  int friendly_king_loc = (m_board->is_white_turn()) ? m_board->get_white_king_loc() : m_board->get_black_king_loc();
  CheckType check = check_type(check_pieces);
  if (check == CheckType::DOUBLE) 
  {
    generate_king_moves(curr_moves, captures_only);
    return;
  }
  else if (check == CheckType::SINGLE) {
    capture_mask = check_pieces;
    int sq = first_set_bit(check_pieces);
    if (is_sliding_piece((*m_board)[sq])) 
    {
      push_mask = opponent_slider_rays_to_square(friendly_king_loc);
    }
    else 
    {
      push_mask = 0;
    }
  }

  bool pawn_check = (check_pieces & (m_board->get_piece_bitboard(WHITE | PAWN) | m_board->get_piece_bitboard(BLACK | PAWN))) != 0;
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
  int perspective = m_board->is_white_turn() ? 1 : -1;
  // maybe add a bonus for castling moves
  // add recapturing the piece that was last captured as a good bonus to check first
  // bigger bonus for the higher value piece being captured
  // just have the board store the move that was made to get to that position
  // still need to add the least_valued_attacker logic, not exactly sure how to implement
  Move last_move = m_board->get_last_move();
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
    mv_piece = (*m_board)[from];
    if (mv.is_promo()) 
    {
      if (flags == Move::KNIGHT_PROMO || flags == Move::KNIGHT_PROMO_CAPTURE) {
        score += constants::piece_values[constants::WHITE_KNIGHTS_INDEX]; // just use the white knights because positive value
      }
      else if (flags == Move::BISHOP_PROMO || flags == Move::BISHOP_PROMO_CAPTURE) {
        score += constants::piece_values[constants::WHITE_BISHOPS_INDEX];
      }
      else if (flags == Move::ROOK_PROMO || flags == Move::ROOK_PROMO_CAPTURE) {
        score += constants::piece_values[constants::WHITE_ROOKS_INDEX];
      }
      else {
        score += constants::piece_values[constants::WHITE_QUEENS_INDEX];
      }
    }
    /* check recapturing moves */
    if (to == recapture_square) 
    {
      score += 5 * abs(constants::piece_values[utils::index_from_pc(mv_piece)]); // arbitrary multiplication
    }
    else if (mv.is_capture()) {
      // score += see_capture(mv); /* this function isn't fast enough I need incrementally updated attack tables */
      tar_piece = (*m_board)[to];
      if (!is_attacked(to, m_board->get_all_pieces())) 
      {
        score += 5 * abs(constants::piece_values[utils::index_from_pc(tar_piece)]);
      }
      else 
      {
        score += abs(constants::piece_values[utils::index_from_pc(tar_piece)]) - abs(constants::piece_values[utils::index_from_pc(mv_piece)]);    
      }
    }
    /* score moves to squares attacked by pawns */
    else if(PIECE(mv_piece) != PAWN && is_attacked_by_pawn(to)) 
      score -= abs(constants::piece_values[utils::index_from_pc(mv_piece)]); // can play around with this
    
    // done for better endgame move ordering of king moves
    if (PIECE(mv_piece) == KING && m_board->get_piece_bitboard(WHITE | QUEEN) == 0 && m_board->get_piece_bitboard(BLACK | QUEEN) == 0)
    {
      score += perspective * (constants::piece_scores[utils::index_from_pc(mv_piece) + 2][to] - constants::piece_scores[utils::index_from_pc(mv_piece) + 2][from]);
    }
    else 
    {
      score += perspective * (constants::piece_scores[utils::index_from_pc(mv_piece)][to] - constants::piece_scores[utils::index_from_pc(mv_piece)][from]);
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
       (*m_board)[single_move.from()] == (*m_board)[move.from()] &&
       single_move.type() == move.type())
    {
      conflicting_moves.push_back(single_move);
    }
  }

  std::string str_move;
  piece mv_piece = (*m_board)[move.from()];
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
    std::cerr << "Empty notation!\n";
    std::exit(-1);
  }
  notation.erase(remove(notation.begin(), notation.end(), '+'), notation.end());
  std::vector<Move> moves;
  generate_moves(moves);
  // this is so ugly
  if(notation == "O-O") {
    for (Move move : moves) {
      if(move.type() == Move::KING_SIDE_CASTLE) return move;
    }
  }
  else if(notation == "O-O-O") {
    for (Move move : moves) {
      if(move.type() == Move::QUEEN_SIDE_CASTLE) return move;
    }
  }
  piece mv_piece;
  char c = notation[0];
  if(isupper(c)) {
    mv_piece = piece_from_move_char(c);
    notation = notation.substr(1, notation.length() - 1);
  }
  else mv_piece = PAWN;
  if(m_board->is_white_turn()) mv_piece |= WHITE;
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

  if(m_board->is_white_turn()) {mv_piece |= WHITE; promotion_piece |= WHITE;}
  else {mv_piece |= BLACK; promotion_piece |= BLACK;}
  
  notation.erase(remove(notation.begin(), notation.end(), 'x'), notation.end());

  int target_rank;
  int target_file;
  int start_rank = -1;
  int start_file = -1;
  int target_square;

  // std::cout << notation << endl;
  if(notation.length() == 2) { // no move conflict
    target_rank = constants::RANKS.find(notation[1]);
    target_file = constants::FILES.find(notation[0]);
  }
  else if(notation.length() == 3) { // some conflict
    target_rank = constants::RANKS.find(notation[2]);
    target_file = constants::FILES.find(notation[1]);

    if(isalpha(notation[0])) {
      start_file = constants::FILES.find(notation[0]);
    }
    else {
      start_rank = constants::RANKS.find(notation[0]);
    }
  }
  else {
    target_rank = constants::RANKS.find(notation[3]);
    target_file = constants::FILES.find(notation[2]);
    
    start_file = constants::FILES.find(notation[0]);
    start_rank = constants::RANKS.find(notation[1]);
  }
  target_square = target_rank * 8 + target_file;

  for (Move move : moves) {
    if(move.to() == target_square && (*m_board)[move.from()] == mv_piece) {
      if(start_rank == -1 && start_file == -1) return move;
      if(start_rank == -1 && start_file == utils::file(move.from())) return move;
      if(start_rank == utils::rank(move.from()) && start_file == -1) return move;
      if(start_rank == utils::rank(move.from()) && start_file == utils::file(move.from())) return move;
    }
  }
  std::cout << "Move: " << notation_copy << std::endl;
  std::cout << m_board->to_string();
  int x;
  std::cout << "No match found!" << std::endl;
  std::cin >> x;
  std::exit(-1); // should match to a move
}

std::string MoveGenerator::move_to_long_algebraic(Move move) const
{
  std::string result;
  
  int from_file = utils::file(move.from());
  int from_rank = utils::rank(move.from());
  
  int to_file = utils::file(move.to());
  int to_rank = utils::rank(move.to());

  result.push_back(constants::FILES[from_file]);
  result.push_back(constants::RANKS[from_rank]);
  result.push_back(constants::FILES[to_file]);
  result.push_back(constants::RANKS[to_rank]);

  int type = move.type();
  if (type == Move::KNIGHT_PROMO || type == Move::KNIGHT_PROMO_CAPTURE)
  {
    result.push_back('n');
  }
  else if (type == Move::BISHOP_PROMO || type == Move::BISHOP_PROMO_CAPTURE)
  {
    result.push_back('b');
  }
  else if (type == Move::ROOK_PROMO || type == Move::ROOK_PROMO_CAPTURE)
  {
    result.push_back('r');
  }
  else if (type == Move::QUEEN_PROMO || type == Move::QUEEN_PROMO_CAPTURE)
  {
    result.push_back('q');
  }
  return result;
}

Move MoveGenerator::move_from_long_algebraic(std::string notation) const
{
  int from_file = constants::FILES.find(notation[0]);
  int from_rank = constants::RANKS.find(notation[1]);
  int to_file = constants::FILES.find(notation[2]);
  int to_rank = constants::RANKS.find(notation[3]);

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
        flags = Move::QUEEN_PROMO;
        flags_capture = Move::QUEEN_PROMO_CAPTURE;
        break;
      case 'b':
        flags = Move::BISHOP_PROMO;
        flags_capture = Move::BISHOP_PROMO_CAPTURE;
        break;
      case 'n':
        flags = Move::KNIGHT_PROMO;
        flags_capture = Move::KNIGHT_PROMO_CAPTURE;
        break;
      case 'r':
        flags = Move::ROOK_PROMO;
        flags_capture = Move::ROOK_PROMO_CAPTURE;
        break;
      default:
        exit(1);
    }
  }

  std::vector<Move> moves;
  generate_moves(moves);

  for (Move move : moves)
  {
    /* check for correct starting and ending square and promotion */
    if (
        (utils::file(move.from()) == from_file) &&
        (utils::rank(move.from()) == from_rank) &&
        (utils::file(move.to()) == to_file) &&
        (utils::rank(move.to()) == to_rank) &&
        (!is_promo || move.type() == flags || move.type() == flags_capture)
       )
    {
      return move;
    }
  }
  std::cerr << "Move not found!\n";
  return Move::NO_MOVE;
}

void MoveGenerator::sort_by_long_algebraic_notation(std::vector<Move>& moves) const
{
  sort(moves.begin(), moves.end(), 
      [this](Move a, Move b) {
        return move_to_long_algebraic(a) < move_to_long_algebraic(b);
      } 
  );
}

MoveGenerator::Pin MoveGenerator::get_pinned_pieces(int friendly_king_loc) const
{
  Pin pin;
  pin.pinned_pieces = 0;
  bitboard curr_pin;
  int pinned_piece_loc;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  bitboard friendly_pieces;
  if(m_board->is_white_turn()) {
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
    friendly_pieces = m_board->get_white_pieces();
  }
  else {
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
    friendly_pieces = m_board->get_black_pieces();
  }
  bitboard king_rook_attacks = lut.get_rook_attacks(friendly_king_loc, m_board->get_all_pieces());
  bitboard king_bishop_attacks = lut.get_bishop_attacks(friendly_king_loc, m_board->get_all_pieces());
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
    rook_attacks = lut.get_rook_attacks(pc_loc, m_board->get_all_pieces());
    curr_pin = rook_attacks & king_rook_attacks & friendly_pieces;
    if(curr_pin){
      pin.pinned_pieces |= curr_pin;
      pinned_piece_loc = first_set_bit(curr_pin);
      pin.ray_at_sq[pinned_piece_loc] = lut.get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
    }
    REMOVE_FIRST(opponent_rooks);
  }
  while(opponent_bishops) {
    pc_loc = first_set_bit(opponent_bishops);
    pc_rank = utils::rank(pc_loc);
    pc_file = utils::file(pc_loc);
    pc_diag = pc_rank - pc_file;
    pc_antidiag = pc_rank + pc_file;
    if(!(pc_diag == king_diag || pc_antidiag == king_antidiag)) {
      REMOVE_FIRST(opponent_bishops);
      continue;
    }
    bishop_attacks = lut.get_bishop_attacks(pc_loc, m_board->get_all_pieces());
    curr_pin = bishop_attacks & king_bishop_attacks & friendly_pieces;
    if(curr_pin){
      pin.pinned_pieces |= curr_pin;
      pinned_piece_loc = first_set_bit(curr_pin);
      pin.ray_at_sq[pinned_piece_loc] = lut.get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
    }
    REMOVE_FIRST(opponent_bishops);
  }
  while(opponent_queens) {
    pc_loc = first_set_bit(opponent_queens);
    pc_rank = utils::rank(pc_loc);
    pc_file = utils::file(pc_loc);
    pc_diag = pc_rank - pc_file;
    pc_antidiag = pc_rank + pc_file;
    if(pc_rank == king_rank || pc_file == king_file) {
      queen_attacks = lut.get_rook_attacks(pc_loc, m_board->get_all_pieces());
      curr_pin = queen_attacks & king_rook_attacks & friendly_pieces;
      if(curr_pin){
        pin.pinned_pieces |= curr_pin;
        pinned_piece_loc = first_set_bit(curr_pin);
        pin.ray_at_sq[pinned_piece_loc] = lut.get_ray_from_rook_to_king(pc_loc, friendly_king_loc);
      }
    }
    else if(pc_diag == king_diag || pc_antidiag == king_antidiag){
      queen_attacks = lut.get_bishop_attacks(pc_loc, m_board->get_all_pieces());
      curr_pin = queen_attacks & king_bishop_attacks & friendly_pieces;
      if(curr_pin){
        pin.pinned_pieces |= curr_pin;
        pinned_piece_loc = first_set_bit(curr_pin);
        pin.ray_at_sq[pinned_piece_loc] = lut.get_ray_from_bishop_to_king(pc_loc, friendly_king_loc);
      }
    }
    REMOVE_FIRST(opponent_queens);
  }
  return pin;
}

bitboard MoveGenerator::checking_pieces() const
{
  int friendly_king = (m_board->is_white_turn()) ? m_board->get_white_king_loc() : m_board->get_black_king_loc();
  Move last_move = m_board->get_last_move();
  if(last_move.is_no_move()) {
    return attackers_from_square(friendly_king);
  }
  bool discovered_check = false;

  int to = last_move.to();
  int from = last_move.from();
  int type = last_move.type();

  bitboard checkers = 0;

  piece moved_piece = (*m_board)[to];
  bitboard piece_bit = BIT_FROM_SQ(to);
  switch (PIECE(moved_piece)) {
    case PAWN:
      checkers = lut.get_pawn_attacks(friendly_king, m_board->is_white_turn()) & piece_bit;
      break;
    case KNIGHT:
      checkers = lut.get_knight_attacks(friendly_king) & piece_bit;
      break;
    case BISHOP:
      checkers = lut.get_bishop_attacks(friendly_king, m_board->get_all_pieces()) & piece_bit;
      break;
    case ROOK:
      checkers = lut.get_rook_attacks(friendly_king, m_board->get_all_pieces()) & piece_bit;
      break;
    case QUEEN:
      checkers = lut.get_queen_attacks(friendly_king, m_board->get_all_pieces()) & piece_bit;
      break;
    case KING:
      break; /* the king can never move to check the other king */
  }

  /* there is no possibility for a discovered check here */
  if(type == Move::QUIET_MOVE || type == Move::DOUBLE_PUSH || type == Move::NORMAL_CAPTURE) {
    if(utils::rank(from) != utils::rank(friendly_king) && utils::file(from) != utils::file(friendly_king) &&
       utils::diag(from) != utils::diag(friendly_king) && utils::anti_diag(from) != utils::anti_diag(friendly_king))
       return checkers;
  }


  bitboard opponent_bishops;
  bitboard opponent_rooks;
  bitboard opponent_queens;

  if(m_board->is_white_turn()) {
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
  }
  else {
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
  }
  bitboard diagonal_sliders = opponent_bishops | opponent_queens;
  bitboard cardinal_sliders = opponent_rooks | opponent_queens;

  /* remove the piece that was moved to not double check it */
  diagonal_sliders &= ~BIT_FROM_SQ(to);
  cardinal_sliders &= ~BIT_FROM_SQ(to);

  bitboard bishop_from_king = lut.get_bishop_attacks(friendly_king, m_board->get_all_pieces());
  bitboard attacking_diagonal = bishop_from_king & diagonal_sliders;
  checkers |= attacking_diagonal;
  if(attacking_diagonal)
    discovered_check = true;

  if(!discovered_check){
    bitboard rook_from_king = lut.get_rook_attacks(friendly_king, m_board->get_all_pieces());
    checkers |= rook_from_king & cardinal_sliders;
  }
  return checkers;
}

MoveGenerator::CheckType MoveGenerator::check_type(bitboard checkers) const
{
  if(!checkers) return CheckType::NONE;
  REMOVE_FIRST(checkers);
  if(!checkers) return CheckType::SINGLE;
  return CheckType::DOUBLE;
}

bool MoveGenerator::in_check() const
{
  return check_type(checking_pieces()) != CheckType::NONE;
}

int MoveGenerator::see(int sq) const
{
  int value = 0;
  int attacker_sq = least_valued_attacker_sq(sq, !m_board->is_white_turn());
  if(attacker_sq != constants::NONE) {
    Move capture{attacker_sq, sq, Move::NORMAL_CAPTURE}; /* would this also work for promotions? */
    piece captured_piece = (*m_board)[sq];
    m_board->make_move(capture);
    value = std::max(0, abs(constants::piece_values[utils::index_from_pc(captured_piece)]) - see(sq));
    m_board->unmake_move(capture);
  }
  return value;
}

int MoveGenerator::see_capture(Move capture) const
{
  int value = 0;
  int to = capture.to();
  piece cap_piece = (*m_board)[to];
  m_board->make_move(capture);
  value = std::abs(constants::piece_values[utils::index_from_pc(cap_piece)]) - see(to);
  m_board->unmake_move(capture);
  return value;
}

bool MoveGenerator::is_bad_capture(Move capture) const
{
  piece mv_piece = (*m_board)[capture.from()];
  piece cap_piece = (*m_board)[capture.to()];
  /* if we are capturing a piece of higher material, its probably good */
  if(abs(constants::piece_values[utils::index_from_pc(cap_piece)]) 
     - abs(constants::piece_values[utils::index_from_pc(mv_piece)]) > 50) {
    return false;
  }
  return see_capture(capture) < -50;
}

bool MoveGenerator::pawn_promo_or_close_push(Move move) const
{
  if(move.is_promo()) return true;
  int from = move.from();
  int to = move.to();
  piece moving_piece = (*m_board)[from];
  if(PIECE(moving_piece) != PAWN) return false;
  if(utils::rank(to) == constants::RANK_7 && COLOR(moving_piece) == WHITE) return true;
  if(utils::rank(to) == constants::RANK_2 && COLOR(moving_piece) == BLACK) return true;
  return false;
}

bitboard MoveGenerator::generate_knight_move_bitboard(int knight_sq, bool captures_only) const
{
  bitboard knight_attacks = lut.get_knight_attacks(knight_sq);
  if(captures_only) {
    bitboard opponent_pieces = (m_board->is_white_turn()) ? m_board->get_black_pieces() : m_board->get_white_pieces();
    return knight_attacks & opponent_pieces;
  }
  bitboard own_pieces = (m_board->is_white_turn()) ? m_board->get_white_pieces() : m_board->get_black_pieces();
  return knight_attacks & ~own_pieces;
}

bitboard MoveGenerator::generate_king_move_bitboard(int king_sq, bool captures_only) const
{
  bitboard own_pieces = (m_board->is_white_turn()) ? m_board->get_white_pieces() : m_board->get_black_pieces();
  bitboard opponent_pieces = (m_board->is_white_turn()) ? m_board->get_black_pieces() : m_board->get_white_pieces();
  
  bitboard king_attacks = lut.get_king_attacks(king_sq);
  bitboard king_pseudomoves = (captures_only) ? (king_attacks & opponent_pieces) : (king_attacks & ~own_pieces);

  if(!king_pseudomoves) return 0; // if the king has no pseudolegal moves, it cannot castle

  bitboard king_legal_moves = 0;
  bitboard blocking_pieces = m_board->get_all_pieces() & ~BIT_FROM_SQ(king_sq); // the king cannot block the attack on a square behind it

  while(king_pseudomoves) {
    int loc = first_set_bit(king_pseudomoves);
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

  int white_king_sq_1 = constants::F1;
  int white_king_sq_2 = constants::G1;
  int white_queen_sq_1 = constants::D1;
  int white_queen_sq_2 = constants::C1;
  int white_queen_sq_3 = constants::B1; // this square is allowed to be attacked

  int black_king_sq_1 = constants::F8;
  int black_king_sq_2 = constants::G8;
  int black_queen_sq_1 = constants::D8;
  int black_queen_sq_2 = constants::C8;
  int black_queen_sq_3 = constants::B8; // this square is allowed to be attacked

  bitboard king_castle = 0;
  if(m_board->is_white_turn() && m_board->get_white_king_loc() == constants::E1 && !is_attacked(constants::E1, blocking_pieces)) {
    if(m_board->can_white_king_side_castle() && (*m_board)[constants::H1] == (WHITE | ROOK)) {
      if((*m_board)[white_king_sq_1] == EMPTY &&
         (*m_board)[white_king_sq_2] == EMPTY) {
           if(!is_attacked(white_king_sq_1, blocking_pieces) &&
            !is_attacked(white_king_sq_2, blocking_pieces))
            {
              king_castle |= w_king_side_castle;
            }
            
         }
    }

    if(m_board->can_white_queen_side_castle() && (*m_board)[constants::A1] == (WHITE | ROOK)) {
      if((*m_board)[white_queen_sq_1] == EMPTY &&
         (*m_board)[white_queen_sq_2] == EMPTY &&
         (*m_board)[white_queen_sq_3] == EMPTY) {
           if(!is_attacked(white_queen_sq_1, blocking_pieces) &&
            !is_attacked(white_queen_sq_2, blocking_pieces))
            king_castle |= w_queen_side_castle;
         }
    } 
  }
  else if (!m_board->is_white_turn() && m_board->get_black_king_loc() == constants::E8 && !is_attacked(constants::E8, blocking_pieces)) {
    if(m_board->can_black_king_side_castle() && (*m_board)[constants::H8] == (BLACK | ROOK)) {
      if((*m_board)[black_king_sq_1] == EMPTY &&
         (*m_board)[black_king_sq_2] == EMPTY) {
           if(!is_attacked(black_king_sq_1, blocking_pieces) &&
            !is_attacked(black_king_sq_2, blocking_pieces))
            king_castle |= b_king_side_castle;
         }
    }

    if(m_board->can_black_queen_side_castle() && (*m_board)[constants::A8] == (BLACK | ROOK)) {
      if((*m_board)[black_queen_sq_1] == EMPTY &&
         (*m_board)[black_queen_sq_2] == EMPTY &&
         (*m_board)[black_queen_sq_3] == EMPTY) {
           if(!is_attacked(black_queen_sq_1, blocking_pieces) &&
            !is_attacked(black_queen_sq_2, blocking_pieces))
            king_castle |= b_queen_side_castle;
         }
    } 
  }
  return king_legal_moves | king_castle;
}

bitboard MoveGenerator::generate_pawn_move_bitboard(int pawn_sq, bool captures_only) const
{
  bitboard enemy_pieces;
  bitboard all_pieces = m_board->get_all_pieces();
  bitboard captures;
  bitboard forward_moves;
  bitboard forward_one;
  bitboard forward_two;
  bitboard en_passant_capture;
  int en_passant_sq = m_board->get_en_passant_sq();
  bitboard en_passant_bit = 0; // default it to zero
  size_t rank = utils::rank(pawn_sq);
  bitboard opponent_rooks;
  bitboard opponent_queens;
  bitboard attackers;
  bitboard side_attackers;
  bitboard board_without_pawns;
  bitboard white_pawn_attacks;
  bitboard black_pawn_attacks;

  if(en_passant_sq != constants::NONE) {
    en_passant_bit =  BIT_FROM_SQ(en_passant_sq); // used to and with attack pattern
  }

  if(m_board->is_white_turn()) {
    enemy_pieces = m_board->get_black_pieces();
    white_pawn_attacks = lut.get_pawn_attacks(pawn_sq, true);
    captures = white_pawn_attacks & enemy_pieces;

    en_passant_capture = white_pawn_attacks & en_passant_bit;
    if(en_passant_capture && rank == utils::rank(m_board->get_white_king_loc())){
      opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
      opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
      board_without_pawns = m_board->get_all_pieces() & ~(BIT_FROM_SQ(pawn_sq)) & ~(en_passant_bit >> 8);
      attackers = lut.get_rook_attacks(m_board->get_white_king_loc(), board_without_pawns) & (opponent_rooks | opponent_queens);
      side_attackers = attackers & lut.get_rank_mask(utils::rank(pawn_sq));
      if(side_attackers) {
        en_passant_capture = 0;
      }
    }

    if(captures_only) return captures | en_passant_capture;

    forward_one = lut.get_pawn_pushes(pawn_sq, true) & ~all_pieces;
    forward_two = 0;
    if(rank == constants::RANK_2 && forward_one) {
      forward_two = lut.get_pawn_pushes(pawn_sq + 8, true) & ~all_pieces;
    }
    forward_moves = forward_one | forward_two;
  }
  else {
    enemy_pieces = m_board->get_white_pieces();
    black_pawn_attacks = lut.get_pawn_attacks(pawn_sq, false);
    captures = black_pawn_attacks & enemy_pieces;

    en_passant_capture = black_pawn_attacks & en_passant_bit;
    if(en_passant_capture && rank == utils::rank(m_board->get_black_king_loc())){
      opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
      opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
      board_without_pawns = m_board->get_all_pieces() & ~(BIT_FROM_SQ(pawn_sq)) & ~(en_passant_bit << 8);
      attackers = lut.get_rook_attacks(m_board->get_black_king_loc(), board_without_pawns) & (opponent_rooks | opponent_queens);
      side_attackers = attackers & lut.get_rank_mask(utils::rank(pawn_sq));
      if(side_attackers) {
        en_passant_capture = 0;
      }
    }

    if(captures_only) return captures | en_passant_capture;

    forward_one = lut.get_pawn_pushes(pawn_sq, false) & ~all_pieces;
    forward_two = 0;
    if(rank == constants::RANK_7 && forward_one) {
      forward_two = lut.get_pawn_pushes(pawn_sq - 8, false) & ~all_pieces;
    }
    forward_moves = forward_one | forward_two;
  }
  return captures | forward_moves | en_passant_capture;
}

bitboard MoveGenerator::generate_rook_move_bitboard(int rook_sq, bool captures_only) const
{
  bitboard rook_attacks = lut.get_rook_attacks(rook_sq, m_board->get_all_pieces());
  if(captures_only) {
    bitboard opponent_pieces = (m_board->is_white_turn()) ? m_board->get_black_pieces() : m_board->get_white_pieces();
    return rook_attacks & opponent_pieces;
  }
  bitboard own_pieces = (m_board->is_white_turn()) ? m_board->get_white_pieces() : m_board->get_black_pieces();
  return rook_attacks & ~own_pieces;
}

bitboard MoveGenerator::generate_bishop_move_bitboard(int bishop_sq, bool captures_only) const
{
  bitboard bishop_attacks = lut.get_bishop_attacks(bishop_sq, m_board->get_all_pieces());
  if(captures_only) {
    bitboard opponent_pieces = (m_board->is_white_turn()) ? m_board->get_black_pieces() : m_board->get_white_pieces();
    return bishop_attacks & opponent_pieces;
  }
  bitboard own_pieces = (m_board->is_white_turn()) ? m_board->get_white_pieces() : m_board->get_black_pieces();
  return bishop_attacks & ~own_pieces;
}

bitboard MoveGenerator::generate_queen_move_bitboard(int queen_sq, bool captures_only) const
{
  return generate_rook_move_bitboard(queen_sq, captures_only)
       | generate_bishop_move_bitboard(queen_sq, captures_only);
}


void MoveGenerator::generate_king_moves(std::vector<Move> &curr_moves, bool captures_only) const
{
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard kings;
  bitboard opponent_pieces;
  if (m_board->is_white_turn()) {
    kings = m_board->get_piece_bitboard(WHITE | KING);
    opponent_pieces = m_board->get_black_pieces();
  }
  else {
    kings = m_board->get_piece_bitboard(BLACK | KING);
    opponent_pieces = m_board->get_white_pieces();
  }
  bitboard king_moves;
  while(kings) {
    from = first_set_bit(kings);
    king_moves = generate_king_move_bitboard(from, captures_only);
    while(king_moves) {
      to = first_set_bit(king_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to - from == 2) { // king side castle
        curr_moves.push_back(Move{from, to, Move::KING_SIDE_CASTLE});
      }
      else if (to - from == -2) { // queen side castle
        curr_moves.push_back(Move{from, to, Move::QUEEN_SIDE_CASTLE});
      } 
      else{
        if(to_bit & opponent_pieces) flags = Move::NORMAL_CAPTURE;
        else                         flags = Move::QUIET_MOVE;
        curr_moves.push_back(Move{from, to, flags});
      }
      
      REMOVE_FIRST(king_moves);
    }
    REMOVE_FIRST(kings);
  }
}

void MoveGenerator::generate_knight_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only) const
{
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard knights;
  bitboard opponent_pieces;
  if (m_board->is_white_turn()) {
    knights = m_board->get_piece_bitboard(WHITE | KNIGHT);
    opponent_pieces = m_board->get_black_pieces();
  }
  else {
    knights = m_board->get_piece_bitboard(BLACK | KNIGHT);
    opponent_pieces = m_board->get_white_pieces();
  }
  bitboard knight_moves;
  bitboard knight_bit;
  while(knights) {
    from = first_set_bit(knights);
    knight_bit = BIT_FROM_SQ(from);
    if(knight_bit & pin.pinned_pieces) {
      REMOVE_FIRST(knights);
      continue;
    } // pinned knights cannot move at all
    knight_moves = generate_knight_move_bitboard(from, captures_only) & check_mask;
    
    while(knight_moves) {
      to = first_set_bit(knight_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = Move::NORMAL_CAPTURE;
      else                         flags = Move::QUIET_MOVE;
      curr_moves.push_back(Move{from, to, flags});
      REMOVE_FIRST(knight_moves);
    }
    REMOVE_FIRST(knights);
  }
}

void MoveGenerator::generate_pawn_moves(std::vector<Move> &curr_moves, bitboard check_mask, bool pawn_check, Pin &pin, bool captures_only) const
{
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard pawns;
  bitboard opponent_pieces; // used for captures only
  if (m_board->is_white_turn()) {
    pawns = m_board->get_piece_bitboard(WHITE | PAWN);
    opponent_pieces = m_board->get_black_pieces();
  }
  else {
    pawns = m_board->get_piece_bitboard(BLACK | PAWN);
    opponent_pieces = m_board->get_white_pieces();
  }

  bitboard pawn_moves;
  bitboard pawn_bit;
  bitboard en_passant_bit = 0;
  if(m_board->get_en_passant_sq() != constants::NONE) {
    en_passant_bit = BIT_FROM_SQ(m_board->get_en_passant_sq());
    if(pawn_check) {
      check_mask |= en_passant_bit;
    }
  }
  while(pawns) {
    from = first_set_bit(pawns);
    pawn_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(pawn_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[from];
    pawn_moves = generate_pawn_move_bitboard(from, captures_only) & check_mask & pin_mask;
    while(pawn_moves) {
      to = first_set_bit(pawn_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & en_passant_bit)         flags = Move::EN_PASSANT_CAPTURE;
      else if(to_bit & opponent_pieces)   flags = Move::NORMAL_CAPTURE;
      else                                flags = Move::QUIET_MOVE;
      int to_rank = utils::rank(to);
      if(to_rank == constants::RANK_8 || to_rank == constants::RANK_1) {
        if(utils::file(to) != utils::file(from)) {
          curr_moves.push_back(Move{from, to, Move::KNIGHT_PROMO_CAPTURE}); // or this on to flag because we check for captures prior to this
          curr_moves.push_back(Move{from, to, Move::BISHOP_PROMO_CAPTURE});
          curr_moves.push_back(Move{from, to, Move::ROOK_PROMO_CAPTURE});
          curr_moves.push_back(Move{from, to, Move::QUEEN_PROMO_CAPTURE});
        }
        else {
          curr_moves.push_back(Move{from, to, Move::KNIGHT_PROMO});
          curr_moves.push_back(Move{from, to, Move::BISHOP_PROMO});
          curr_moves.push_back(Move{from, to, Move::ROOK_PROMO});
          curr_moves.push_back(Move{from, to, Move::QUEEN_PROMO});
        }
        
      }
      else if (abs(from - to) == 16) { // double pawn push
        curr_moves.push_back(Move{from, to, Move::DOUBLE_PUSH});
      }
      else {
        curr_moves.push_back(Move{from, to, flags}); // should already be set to quiet or capture
      }
      REMOVE_FIRST(pawn_moves);
    }
    REMOVE_FIRST(pawns);
  }
}

void MoveGenerator::generate_rook_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only) const
{
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard rooks;
  bitboard opponent_pieces;
  if (m_board->is_white_turn()) {
    rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_pieces = m_board->get_black_pieces();
  }
  else {
    rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_pieces = m_board->get_white_pieces();
  }

  bitboard rook_moves;
  bitboard rook_bit;
  while(rooks) {
    from = first_set_bit(rooks);
    rook_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(rook_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[from];
    rook_moves = generate_rook_move_bitboard(from, captures_only) & check_mask & pin_mask;

    while(rook_moves) {
      to = first_set_bit(rook_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = Move::NORMAL_CAPTURE;
      else                         flags = Move::QUIET_MOVE;
      curr_moves.push_back(Move{from, to, flags});
      REMOVE_FIRST(rook_moves);
    }
    REMOVE_FIRST(rooks);
  }
}

void MoveGenerator::generate_bishop_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only) const
{
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard bishops;
  bitboard opponent_pieces;
  if (m_board->is_white_turn()) {
    bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_pieces = m_board->get_black_pieces();
  }
  else {
    bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_pieces = m_board->get_white_pieces();
  }

  bitboard bishop_moves;
  bitboard bishop_bit;
  while(bishops) {
    from = first_set_bit(bishops);
    bishop_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(bishop_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[from];
    bishop_moves = generate_bishop_move_bitboard(from, captures_only) & check_mask & pin_mask;
    
    while(bishop_moves) {
      to = first_set_bit(bishop_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = Move::NORMAL_CAPTURE;
      else                         flags = Move::QUIET_MOVE;
      curr_moves.push_back(Move{from, to, flags});
      REMOVE_FIRST(bishop_moves);
    }
    REMOVE_FIRST(bishops);
  }
}

void MoveGenerator::generate_queen_moves(std::vector<Move> &curr_moves, bitboard check_mask, Pin &pin, bool captures_only) const
{
  int from;
  int to;
  int flags;
  bitboard to_bit;
  bitboard pin_mask;
  bitboard queens;
  bitboard opponent_pieces;
  if (m_board->is_white_turn()) {
    queens = m_board->get_piece_bitboard(WHITE | QUEEN);
    opponent_pieces = m_board->get_black_pieces();
  }
  else {
    queens = m_board->get_piece_bitboard(BLACK | QUEEN);
    opponent_pieces = m_board->get_white_pieces();
  }

  bitboard queen_moves;
  bitboard queen_bit;
  while(queens) {
    from = first_set_bit(queens);
    queen_bit = BIT_FROM_SQ(from);
    pin_mask = 0xFFFFFFFFFFFFFFFF;
    if(queen_bit & pin.pinned_pieces) pin_mask = pin.ray_at_sq[from];
    queen_moves = generate_queen_move_bitboard(from, captures_only) & check_mask & pin_mask;
    
    while(queen_moves) {
      to = first_set_bit(queen_moves);
      to_bit = BIT_FROM_SQ(to);
      if(to_bit & opponent_pieces) flags = Move::NORMAL_CAPTURE;
      else                         flags = Move::QUIET_MOVE;
      curr_moves.push_back(Move{from, to, flags});
      REMOVE_FIRST(queen_moves);
    }
    REMOVE_FIRST(queens);
  }
}

bitboard MoveGenerator::generate_attack_map(bool white_side) const
{
  bitboard attack_map = 0;
  size_t pc_loc;
  bitboard color_pieces;
  bitboard all_pieces_no_king;
  piece curr_piece;
  piece color;
  if(m_board->is_white_turn()) {
    all_pieces_no_king = m_board->get_all_pieces() & 
              ~m_board->get_piece_bitboard(BLACK | KING);
    color_pieces = m_board->get_white_pieces();
    color = WHITE;
  }
  else {
    all_pieces_no_king = m_board->get_all_pieces() & 
              ~m_board->get_piece_bitboard(WHITE | KING);
    color_pieces = m_board->get_black_pieces();
    color = BLACK;
  }

  while(color_pieces) {
    pc_loc = first_set_bit(color_pieces);
    curr_piece = (*m_board)[pc_loc];
    if(curr_piece == (color | PAWN)) {
      attack_map |= lut.get_pawn_attacks(pc_loc, white_side);
    }
    else if(curr_piece == (color | KNIGHT)) {
      attack_map |= lut.get_knight_attacks(pc_loc);
    }
    else if(curr_piece == (color | BISHOP)) {
      attack_map |= lut.get_bishop_attacks(pc_loc, all_pieces_no_king);
    }
    else if(curr_piece == (color | ROOK)) {
      attack_map |= lut.get_rook_attacks(pc_loc, all_pieces_no_king);
    }
    else if(curr_piece == (color | QUEEN)) {
      attack_map |= lut.get_queen_attacks(pc_loc, all_pieces_no_king);
    }
    else if(curr_piece == (color | KING)) {
      attack_map |= lut.get_king_attacks(pc_loc);
    }
    REMOVE_FIRST(color_pieces);
  }
  return attack_map;
}

piece MoveGenerator::least_valued_attacker(int sq) const
{
  bitboard opponent_knights;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(m_board->is_white_turn()) {
    opponent_knights = m_board->get_piece_bitboard(BLACK | KNIGHT);
    opponent_pawns = m_board->get_piece_bitboard(BLACK | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
  }
  else {
    opponent_knights = m_board->get_piece_bitboard(WHITE | KNIGHT);
    opponent_pawns = m_board->get_piece_bitboard(WHITE | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
  }
  if(lut.get_pawn_attacks(sq, m_board->is_white_turn()) & opponent_pawns) return PAWN;
  if(lut.get_knight_attacks(sq) & opponent_knights) return KNIGHT;
  if(lut.get_bishop_attacks(sq, m_board->get_all_pieces()) & opponent_bishops) return BISHOP;
  if(lut.get_rook_attacks(sq, m_board->get_all_pieces()) & opponent_rooks) return ROOK;
  if(lut.get_queen_attacks(sq, m_board->get_all_pieces()) & opponent_queens) return QUEEN;
  // if(get_king_attacks(sq) & opponent_kings) return KING; /* not sure what to do here */
  return EMPTY;
}

int MoveGenerator::least_valued_attacker_sq(int sq, bool white_turn) const
{
  bitboard opponent_knights;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(m_board->is_white_turn()) {
    opponent_knights = m_board->get_piece_bitboard(BLACK | KNIGHT);
    opponent_pawns = m_board->get_piece_bitboard(BLACK | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
  }
  else {
    opponent_knights = m_board->get_piece_bitboard(WHITE | KNIGHT);
    opponent_pawns = m_board->get_piece_bitboard(WHITE | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
  }
  bitboard pawns = lut.get_pawn_attacks(sq, white_turn) & opponent_pawns;
  if(pawns) return first_set_bit(pawns);

  bitboard knights = lut.get_knight_attacks(sq) & opponent_knights;
  if(knights) return first_set_bit(knights);

  bitboard bishops = lut.get_bishop_attacks(sq, m_board->get_all_pieces()) & opponent_bishops;
  if(bishops) return first_set_bit(bishops);

  bitboard rooks = lut.get_rook_attacks(sq, m_board->get_all_pieces()) & opponent_rooks;
  if(rooks) return first_set_bit(rooks);

  bitboard queens = lut.get_queen_attacks(sq, m_board->get_all_pieces()) & opponent_queens;
  if(queens) return first_set_bit(queens);
  return constants::NONE;
}

bool MoveGenerator::is_attacked_by_pawn(int sq) const
{
  bitboard opponent_pawns;
  if(m_board->is_white_turn())
    opponent_pawns = m_board->get_piece_bitboard(BLACK | PAWN);
  else
    opponent_pawns = m_board->get_piece_bitboard(WHITE | PAWN);
  if(lut.get_pawn_attacks(sq, m_board->is_white_turn()) & opponent_pawns) return true;
  return false;
}

bool MoveGenerator::is_attacked(int sq, bitboard blockers) const
{
  bitboard opponent_knights;
  bitboard opponent_kings;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(m_board->is_white_turn()) {
    opponent_knights = m_board->get_piece_bitboard(BLACK | KNIGHT);
    opponent_kings = m_board->get_piece_bitboard(BLACK | KING);
    opponent_pawns = m_board->get_piece_bitboard(BLACK | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
  }
  else {
    opponent_knights = m_board->get_piece_bitboard(WHITE | KNIGHT);
    opponent_kings = m_board->get_piece_bitboard(WHITE | KING);
    opponent_pawns = m_board->get_piece_bitboard(WHITE | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
  }
  if(lut.get_bishop_attacks(sq, blockers) & opponent_bishops) return true;
  if(lut.get_rook_attacks(sq, blockers) & opponent_rooks) return true;
  if(lut.get_knight_attacks(sq) & opponent_knights) return true;
  if(lut.get_pawn_attacks(sq, m_board->is_white_turn()) & opponent_pawns) return true;
  if(lut.get_queen_attacks(sq, blockers) & opponent_queens) return true;
  if(lut.get_king_attacks(sq) & opponent_kings) return true;
  return false;
}

bitboard MoveGenerator::attackers_from_square(int sq) const
{
  bitboard attackers = 0;
  bitboard opponent_knights;
  bitboard opponent_kings;
  bitboard opponent_pawns;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(m_board->is_white_turn()) {
    opponent_knights = m_board->get_piece_bitboard(BLACK | KNIGHT);
    opponent_kings = m_board->get_piece_bitboard(BLACK | KING);
    opponent_pawns = m_board->get_piece_bitboard(BLACK | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
  }
  else {
    opponent_knights = m_board->get_piece_bitboard(WHITE | KNIGHT);
    opponent_kings = m_board->get_piece_bitboard(WHITE | KING);
    opponent_pawns = m_board->get_piece_bitboard(WHITE | PAWN);
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
  }
  attackers |= lut.get_knight_attacks(sq) & opponent_knights;
  attackers |= lut.get_king_attacks(sq) & opponent_kings;
  attackers |= lut.get_pawn_attacks(sq, m_board->is_white_turn()) & opponent_pawns;
  attackers |= lut.get_rook_attacks(sq, m_board->get_all_pieces()) & opponent_rooks;
  attackers |= lut.get_bishop_attacks(sq, m_board->get_all_pieces()) & opponent_bishops;
  attackers |= lut.get_queen_attacks(sq, m_board->get_all_pieces()) & opponent_queens;
  return attackers;
}

bitboard MoveGenerator::opponent_slider_rays_to_square(int sq) const
{
  bitboard res = 0;
  int attacker_loc;
  bitboard attackers;
  bitboard opponent_rooks;
  bitboard opponent_bishops;
  bitboard opponent_queens;
  if(m_board->is_white_turn()) {
    opponent_rooks = m_board->get_piece_bitboard(BLACK | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(BLACK | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(BLACK | QUEEN);
  }
  else {
    opponent_rooks = m_board->get_piece_bitboard(WHITE | ROOK);
    opponent_bishops = m_board->get_piece_bitboard(WHITE | BISHOP);
    opponent_queens = m_board->get_piece_bitboard(WHITE | QUEEN);
  }
  bitboard rook_attacks_from_sq = lut.get_rook_attacks(sq, m_board->get_all_pieces());
  attackers = rook_attacks_from_sq & (opponent_rooks | opponent_queens);
  while(attackers) {
    attacker_loc = first_set_bit(attackers);
    res |= lut.get_rook_attacks(attacker_loc, m_board->get_all_pieces()) & rook_attacks_from_sq;
    REMOVE_FIRST(attackers);    
  }
  bitboard bishop_attacks_from_sq = lut.get_bishop_attacks(sq, m_board->get_all_pieces());
  attackers = bishop_attacks_from_sq & (opponent_bishops | opponent_queens);
  while(attackers) {
    attacker_loc = first_set_bit(attackers);
    res |= lut.get_bishop_attacks(attacker_loc, m_board->get_all_pieces()) & bishop_attacks_from_sq;
    REMOVE_FIRST(attackers);
  }
  return res;
}

