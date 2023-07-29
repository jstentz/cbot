#include "include/search.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/move.h"
#include "include/attacks.h"
#include "include/tt.h"
#include "include/openings.h"
#include "include/evaluation.h"
#include "include/constants.h"
#include "include/utils.h"

#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>

Searcher::Searcher(Board::Ptr board) : m_board{board}, m_move_gen{board}, m_tt{constants::SEARCH_TT_SIZE}, m_evaluator{m_board} {}

uint64_t Searcher::perft(int depth)
{
  std::vector<Move> moves;
  m_move_gen.generate_moves(moves);

  uint64_t total_nodes = 0;
  uint64_t nodes_from_move;
  m_move_gen.sort_by_long_algebraic_notation(moves);
  for (Move& move : moves) {
    std::cout << m_move_gen.move_to_long_algebraic(move) << ": ";
    m_board->make_move(move);
    nodes_from_move = num_nodes_bulk(depth - 1);
    total_nodes += nodes_from_move;
    std::cout << nodes_from_move << std::endl;
    m_board->unmake_move(move);
  }
  std::cout << std::endl << "Nodes searched: " << total_nodes << std::endl << std::endl;
  return total_nodes;
}

uint64_t Searcher::num_nodes_bulk(int depth)
{
  std::vector<Move> moves;
  m_move_gen.generate_moves(moves);
  if (depth == 1) {
    return moves.size();
  }
  else if (depth == 0) {
    return 1;
  }

  uint64_t total_moves = 0;
  for (Move& move : moves) {
    m_board->make_move(move);
    total_moves += num_nodes_bulk(depth - 1); 
    m_board->unmake_move(move);
  }
  return total_moves;
}

uint64_t Searcher::num_nodes(int depth)
{
  if (depth == 0) {
    return 1;
  }

  uint64_t total_moves = 0;
  std::vector<Move> moves;
  m_move_gen.generate_moves(moves);
  for (Move& move : moves) {
    m_board->make_move(move);
    total_moves += num_nodes(depth - 1); 
    m_board->unmake_move(move);
  }
  return total_moves;
}

/// TODO: Need to make this interruptable from the outside
/// TODO: rename this to be the iterative deepening search 
void Searcher::find_best_move()
{
  m_move_gen.clear_killers(); // clear the killer moves
  m_best_move = Move::NO_MOVE;

  for (size_t depth = 1; depth < INT_MAX; depth++) 
  {
    m_best_move_this_iteration = Move::NO_MOVE;
    m_best_score_this_iteration = INT_MIN + 1;
    search(0, depth, INT_MIN + 1, INT_MAX, true, true);

    if (!m_best_move_this_iteration.is_no_move())
    {

      m_best_move = m_best_move_this_iteration;
      m_best_score = m_best_score_this_iteration;
      std::cout << "info depth " << depth << " currmove " << m_move_gen.move_to_long_algebraic(m_best_move) 
                << " cp " << m_best_score * (m_board->is_white_turn() ? 1 : -1) << std::endl;
    }

    if (m_abort_search)
    {
      break;
    }
    
  }
  m_abort_search = false; // reset
}

// spawns a thread to do the searching and returns 
void Searcher::ponder()
{
  // start searching
  m_search_thread = std::thread{&Searcher::find_best_move, this};
}

void Searcher::stop()
{
  abort_search();
  if (m_search_thread.joinable())
  {
    m_search_thread.join();
  }
  std::cout << "bestmove " << m_move_gen.move_to_long_algebraic(get_best_move()) << std::endl;
}

void Searcher::find_best_move_timed(int time)
{
  /// TODO: move this into a function in the search class 
  std::thread t{&Searcher::find_best_move, this};
  std::this_thread::sleep_for(std::chrono::milliseconds(time));
  abort_search();
  if (t.joinable())
  {
    t.join();
  }
  std::cout << "bestmove " << m_move_gen.move_to_long_algebraic(get_best_move()) << std::endl;
}

int Searcher::qsearch(int alpha, int beta)
{
  std::vector<Move> captures;
  uint64_t h = m_board->get_hash();

  /**
   * Since none of these captures are forced, meaning a player doesn't
   * have to make that capture, we can use this evaluation to represent
   * them not taking the piece. We will see if it is better or worse for
   * them to make that capture.
   */
  int stand_pat = m_evaluator.evaluate(alpha, beta); // fall back evaluation
  if(stand_pat >= beta) return beta;
  if(alpha < stand_pat) alpha = stand_pat;

  m_move_gen.generate_moves(captures, true); // true flag generates only captures
  m_move_gen.order_moves(captures, Move::NO_MOVE); /* I could make an order capture functions that I call here to not waste time */
  for (Move& capture : captures) {
    /* delta pruning helps to stop searching helpless nodes */
    // piece captured_piece = b.sq_board[TO(capture)];
    if(m_move_gen.is_bad_capture(capture)) /* don't consider captures that are bad */
      continue;
    m_board->make_move(capture);
    int evaluation = -qsearch(-beta, -alpha);
    m_board->unmake_move(capture);

    if(evaluation >= beta) return beta;
    if(evaluation > alpha) alpha = evaluation;
  }
  return alpha;
}

int Searcher::search(int ply_from_root, int depth, int alpha, int beta, bool is_pv, bool can_null)
{
  if (m_abort_search)
  {
    return 0;
  }

  TranspositionTable::Flags flags = TranspositionTable::ALPHA;
  uint64_t h = m_board->get_hash();
  if (ply_from_root > 0)
  {
    std::optional<int> tt_score = m_tt.fetch_score(h, depth, ply_from_root, alpha, beta);
    if (tt_score)
    {
      return tt_score.value();
    }

    if(m_board->is_repetition()) 
    {
      return 0;
    }
  }

  if (depth == 0) 
  {
    return qsearch(alpha, beta);
  }

  // if we just made a null move (passed the turn), we cannot be in check
  bool check_flag = can_null ? m_move_gen.in_check() : false;

  /* check extension */
  if (check_flag)
  {
    depth++;
  }

  /* 
    Null Move Pruning:
      - Runs on the assumption that if you give your opponent a free move, and the resulting
      shorter-depth search still causes a cutoff, it is very likely the initial position would have 
      caused a cutoff.
      - Because of ZugZwang in the endgame, where making a null move can be good, this pruning
      is turned off in the endgame.
  */
  if (depth > 2 && 
      can_null && 
      !is_pv && 
      m_board->get_total_material() > constants::ENDGAME_MATERIAL &&
      !check_flag &&
      ply_from_root > 0) 
  {
    int reduce = 2;
    if (depth > 6) 
    {
      reduce = 3;
    }
    m_board->make_nullmove();
    int score = -search(ply_from_root, depth - reduce - 1, -beta, -beta + 1, false, false);
    m_board->unmake_nullmove();

    if (m_abort_search)
    {
      return 0;
    }

    if (score >= beta)
    {
      m_tt.store(h, depth, ply_from_root, TranspositionTable::BETA, beta, Move::NO_MOVE); // this used to be above the if statement
      return beta;
    }
  }

  std::vector<Move> moves;
  m_move_gen.generate_moves(moves);
  m_move_gen.order_moves(moves, (ply_from_root == 0) ? m_best_move : m_tt.fetch_best_move(h), std::make_optional<int>(ply_from_root)); // search the best move if in the top position
  
  Move best_move_this_search;
  int evaluation;
  bool pv_search = true;
  
  for (const auto& move : moves) 
  {
    bool pawn_extension = m_move_gen.pawn_promo_or_close_push(move);
    m_board->make_move(move);
    /*
      Principal Variation Search (PVS):
        - There is only one pathway of moves that are acceptable for both players in any given search.
        - All we have to do in non PV nodes is prove that they are not acceptable for either player.
        - If we are wrong about being in a PV node, a costly re-search is required.
    */

    if (pv_search) 
    {
      evaluation = -search(ply_from_root + 1, depth - 1 + pawn_extension, -beta, -alpha, true, true);
    }
    else 
    {
      evaluation = -search(ply_from_root + 1, depth - 1 + pawn_extension, -alpha - 1, -alpha, false, true);
      if (evaluation > alpha) 
      {
        evaluation = -search(ply_from_root + 1, depth - 1 + pawn_extension, -beta, -alpha, true, true);
      }
    }

    m_board->unmake_move(move);

    if (m_abort_search)
    {
      return 0;
    }
    
    if (evaluation >= beta) 
    {
      m_move_gen.insert_killer(ply_from_root, move);
      m_tt.store(h, depth, ply_from_root, TranspositionTable::BETA, beta, move); 
      return beta;
    }
    /* found a new best move here! */
    if (evaluation > alpha) 
    {
      flags = TranspositionTable::EXACT;
      alpha = evaluation;
      best_move_this_search = move;
      /* if we are at the root node, replace the best move we've seen so far */
      if (ply_from_root == 0 && !m_abort_search)
      {
        m_best_move_this_iteration = best_move_this_search;
        m_best_score_this_iteration = evaluation;
      }
    }
    pv_search = false;
  }

  if (moves.empty()) 
  {
    if (check_flag) 
    { /* checkmate */
      alpha = INT_MIN + 1 + ply_from_root; // the deeper in the search we are, the less good the checkmate is
    }
    else 
    {
      /* stalemate! */
      alpha = 0;
    }
    flags = TranspositionTable::EXACT; /* we know the exact score of checkmated or stalemated positions */
  }

  /* store this in the transposition table */
  m_tt.store(h, depth, ply_from_root, flags, alpha, best_move_this_search);
  return alpha;
}

void Searcher::abort_search()
{
  m_abort_search = true;
}

Move Searcher::get_best_move()
{
  return m_best_move;
}