#include "include/evaluation.h"
#include "include/board.h"
#include "include/bitboard.h"
#include "include/pieces.h"
#include "include/attacks.h"
#include "include/utils.h"

#include <cstdlib>

/* 
  can probably incrementally update this value in the future and store it in the board
  can also probably incrementally update number of pieces and positional scores
*/
void calculate_game_phase() {
  int pawn_phase = 0;
  int knight_phase = 1;
  int bishop_phase = 1;
  int rook_phase = 2;
  int queen_phase = 4;
  float total_phase = pawn_phase * 16 + 
            knight_phase * 4 + 
            bishop_phase * 4 + 
            rook_phase * 4 + 
            queen_phase * 2;

  float phase = total_phase;
  /* count the number of each piece */
  int wp = b.piece_counts[WHITE_PAWNS_INDEX];
  int wn = b.piece_counts[WHITE_KNIGHTS_INDEX];
  int wb = b.piece_counts[WHITE_BISHOPS_INDEX];
  int wr = b.piece_counts[WHITE_ROOKS_INDEX];
  int wq = b.piece_counts[WHITE_QUEENS_INDEX];

  int bp = b.piece_counts[BLACK_PAWNS_INDEX];
  int bn = b.piece_counts[BLACK_KNIGHTS_INDEX];
  int bb = b.piece_counts[BLACK_BISHOPS_INDEX];
  int br = b.piece_counts[BLACK_ROOKS_INDEX];
  int bq = b.piece_counts[BLACK_QUEENS_INDEX];

  phase -= wp * pawn_phase;
  phase -= wn * knight_phase;
  phase -= wb * bishop_phase;
  phase -= wr * rook_phase;
  phase -= wq * queen_phase;

  phase -= bp * pawn_phase;
  phase -= bn * knight_phase;
  phase -= bb * bishop_phase;
  phase -= br * rook_phase;
  phase -= bq * queen_phase;

  game_phase = (phase * 256 + (total_phase / 2)) / total_phase;
  return;
}


int mop_up_eval(turn winning_side) {
  // this function assumes that there are no pawns for either side
  // always returns from white's perspective
  int eval;
  square losing_king;
  square winning_king;
  int perspective;

  if(winning_side == W) {
    losing_king = b.black_king_loc;
    winning_king = b.white_king_loc;
    perspective = 1;
  }
  else {
    losing_king = b.white_king_loc;
    winning_king = b.black_king_loc;
    perspective = -1;
  }

  eval = 10 * cmd(losing_king) + 4 * (14 - md(losing_king, winning_king));
  return eval * perspective;
}

/* value returned will be from white's perspective 
   this means that a negative score is good for black */
/* shoud check here for attacking pawns! */
int evaluate_pawns() {
  return 0;
}

int evaluate_knights(bitboard white_king_squares, bitboard black_king_squares) {
  int mobility = 0;
  int attacking_score = 0;

  bitboard white_knights = b.piece_boards[WHITE_KNIGHTS_INDEX];
  bitboard black_knights = b.piece_boards[BLACK_KNIGHTS_INDEX];
  bitboard attacks;
  while(white_knights) {
    attacks = get_knight_attacks((square)first_set_bit(white_knights));
    mobility += pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score += pop_count(attacks & black_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(white_knights);
  }

  while(black_knights) {
    attacks = get_knight_attacks((square)first_set_bit(black_knights));
    mobility -= pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score -= pop_count(attacks & white_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(black_knights);
  }
  return mobility + attacking_score;
}

int evaluate_bishops(bitboard white_king_squares, bitboard black_king_squares) {
  int mobility = 0;
  int attacking_score = 0;

  bitboard white_bishops = b.piece_boards[WHITE_BISHOPS_INDEX];
  bitboard black_bishops = b.piece_boards[BLACK_BISHOPS_INDEX];
  bitboard attacks;
  while(white_bishops) {
    attacks = get_bishop_attacks((square)first_set_bit(white_bishops), b.all_pieces);
    mobility += pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score += pop_count(attacks & black_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(white_bishops);
  }

  while(black_bishops) {
    attacks = get_bishop_attacks((square)first_set_bit(black_bishops), b.all_pieces);
    mobility -= pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score -= pop_count(attacks & white_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(black_bishops);
  }
  return mobility + attacking_score;
}

int evaluate_rooks(bitboard white_king_squares, bitboard black_king_squares) {
  int mobility = 0;
  int attacking_score;
  bitboard white_rooks = b.piece_boards[WHITE_ROOKS_INDEX];
  bitboard black_rooks = b.piece_boards[BLACK_ROOKS_INDEX];
  bitboard attacks;

  while(white_rooks) {
    attacks = get_rook_attacks((square)first_set_bit(white_rooks), b.all_pieces);
    mobility += pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score += pop_count(attacks & black_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(white_rooks);
  }

  while(black_rooks) {
    attacks = get_rook_attacks((square)first_set_bit(black_rooks), b.all_pieces);
    mobility -= pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score -= pop_count(attacks & white_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(black_rooks);
  }
  return mobility + attacking_score;
}

int evaluate_queens(bitboard white_king_squares, bitboard black_king_squares) {
  int mobility = 0;
  int attacking_score;
  bitboard white_queens = b.piece_boards[WHITE_QUEENS_INDEX];
  bitboard black_queens = b.piece_boards[BLACK_QUEENS_INDEX];
  bitboard attacks;

  while(white_queens) {
    attacks = get_queen_attacks((square)first_set_bit(white_queens), b.all_pieces);
    mobility += pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score += pop_count(attacks & black_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(white_queens);
  }

  while(black_queens) {
    attacks = get_queen_attacks((square)first_set_bit(black_queens), b.all_pieces);
    mobility -= pop_count(attacks) * MOBILITY_WEIGHT;
    attacking_score -= pop_count(attacks & white_king_squares) * ATTACKING_WEIGHT;
    REMOVE_FIRST(black_queens);
  }
  return mobility + attacking_score;
}


Evaluator::Evaluator(Board::Ptr board) : m_board{board} {}

int Evaluator::evaluate(int alpha, int beta)
{
  /* widen the margins a bit for lazy evaluation */
  if (!utils::is_mate_score(alpha)) alpha -= LAZY_EVAL_MARGIN; /* prevents underflow */
  if (!utils::is_mate_score(beta))  beta  += LAZY_EVAL_MARGIN; /* prevents overflow  */

  /* probe the eval table */
  std::optional<int> table_score = m_table.fetch(m_board->get_piece_hash(), alpha, beta); /* use the pieces since that's all that matters for eval */
  if (table_score)
    return table_score.value();

  int eval;
  int lazy_eval;
  int middlegame_eval;
  int endgame_eval;
  int perspective = (m_board->is_white_turn()) ? 1 : -1;

  if(!sufficient_checkmating_material()) {
    m_table.store(m_board->get_piece_hash(), TranspositionTable::EXACT, 0);
    return 0;
  }

  float game_phase = calculate_game_phase();

  int white_king_loc = m_board->get_white_king_loc();
  int black_king_loc = m_board->get_white_king_loc();

  int middlegame_positional = m_board->get_positional_score() + 
                constants::piece_scores[constants::WHITE_KINGS_INDEX][white_king_loc] +
                constants::piece_scores[constants::BLACK_KINGS_INDEX][black_king_loc];

  int endgame_positional = m_board->get_positional_score() + 
               constants::piece_scores[constants::WHITE_KINGS_INDEX + 2][white_king_loc] +
               constants::piece_scores[constants::BLACK_KINGS_INDEX + 2][black_king_loc];

  middlegame_eval = middlegame_positional + m_board->get_material_score();
  endgame_eval = endgame_positional + m_board->get_material_score();

  /* in order to do lazy eval, we will see if this exceeds the alpha-beta bounds */
  lazy_eval = (((middlegame_eval * (256 - game_phase)) + (endgame_eval * game_phase)) / 256);
  lazy_eval *= perspective;
  if(lazy_eval >= beta) {
    m_table.store(m_board->get_piece_hash(), TranspositionTable::BETA, beta);
    return beta;
  }
  else if(lazy_eval <= alpha) {
    m_table.store(m_board->get_piece_hash(), TranspositionTable::ALPHA, alpha);
    return alpha;
  }
  /* otherwise we get the exact score */

  /* mop up eval for winning side */
  if(m_board->get_material_score() != 0){
    if(m_board->get_material_score() > 0) endgame_eval += mop_up_eval(true);
    else endgame_eval += mop_up_eval(false);
  }
  
  // /* add a tempo bonus to middle game */
  // if(b.t == W) middlegame_eval += 10;
  // else         middlegame_eval -= 10;

  int queen_moves_from_white_king = pop_count(lut.get_queen_attacks(m_board->get_white_king_loc(), m_board->get_all_pieces()) & ~m_board->get_white_pieces());
  int queen_moves_from_black_king = pop_count(lut.get_queen_attacks(m_board->get_black_king_loc(), m_board->get_all_pieces()) & ~m_board->get_black_pieces());

  middlegame_eval -= queen_moves_from_white_king * 5;
  middlegame_eval += queen_moves_from_black_king * 5;


  /* evaluate the mobility and attacking score of each piece */
  bitboard white_king_squares = BIT_FROM_SQ(white_king_loc) | lut.get_king_attacks(white_king_loc);
  bitboard black_king_squares = BIT_FROM_SQ(black_king_loc) | lut.get_king_attacks(black_king_loc);
  
  int knight_score = evaluate_knights(white_king_squares, black_king_squares);
  int bishop_score = evaluate_bishops(white_king_squares, black_king_squares);
  int rook_score = evaluate_rooks(white_king_squares, black_king_squares);
  int queen_score = evaluate_queens(white_king_squares, black_king_squares);

  /* we care more about the placement of our minor pieces in the middle game */
  middlegame_eval += (knight_score + bishop_score) * 2 + (rook_score + queen_score);
  endgame_eval += (knight_score + bishop_score + rook_score + queen_score);

  /* check for pawn shield */
  // bitboard white_king_area = get_king_attacks(white_king_loc);
  // bitboard black_king_area = get_king_attacks(black_king_loc);

  // int white_pawn_shield_penalty =  60 - 20 * pop_count(white_king_area & b.piece_boards[WHITE_PAWNS_INDEX]);
  // int black_pawn_shield_penalty = -60 + 20 * pop_count(black_king_area & b.piece_boards[BLACK_PAWNS_INDEX]);

  // middlegame_eval -= white_pawn_shield_penalty;
  // middlegame_eval -= black_pawn_shield_penalty;

  eval = (((middlegame_eval * (256 - game_phase)) + (endgame_eval * game_phase)) / 256);

  if(m_board->get_piece_count(WHITE | BISHOP) >= 2) eval += 30; /* bishop pair bonus for white */
  if(m_board->get_piece_count(BLACK | BISHOP) >= 2) eval -= 30; /* bishop pair bonus for black */

  eval *= perspective;

  /* save the evaluation we just made */
  /* here I just need to hash the board's pieces 
  add this after I finish unmake move */
  m_table.store(m_board->get_piece_hash(), TranspositionTable::EXACT, eval); /* use just the pieces again */
  return eval;
}

bool Evaluator::sufficient_checkmating_material()
{
  if(m_board->get_piece_count(WHITE | BISHOP) != 0 ||
     m_board->get_piece_count(BLACK | BISHOP) != 0)
    return true;
  
  if(m_board->get_material_score() >= 400 ||
     m_board->get_material_score() <= -400) {
    return true;
  }
  return false;
}

float Evaluator::calculate_game_phase()
{
  
}
