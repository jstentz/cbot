#include <iostream>
#include <thread>
#include <string>
#include <bits/stdc++.h> 

#include "include/uci.h"
#include "include/search.h"
#include "include/board.h"
#include "include/move.h"
#include "include/tt.h"
#include "include/utils.h"

/// TODO: make it so commands at the wrong time don't work
void UCICommunicator::start_uci_communication()
{
  std::string cmd;
  std::vector<std::string> cmd_list;
  std::string main_cmd;

  /* wait for the uci cmd */
  while (std::getline(std::cin, cmd))
  {
    /* extract command parts */
    cmd_list = utils::split(cmd, ' ');
    if (!cmd_list.size())
      continue;
    main_cmd = cmd_list[0];

    /* condition on main command */
    if (main_cmd == UCI)
    {
      handle_uci();
    }
    else if (main_cmd == ISREADY)
    {
      handle_is_ready();
    }
    else if (main_cmd == UCINEWGAME)
    {
      handle_new_game();
    }
    else if (main_cmd == POSITION)
    {
      handle_position(cmd_list, cmd);
    }
    else if (main_cmd == GO)
    {
      handle_go(cmd_list, cmd);
    }
    else if (main_cmd == QUIT)
    {
      handle_quit(); 
    }
    else if (main_cmd == VERIFY)
    {
      handle_verify(cmd_list);
    }
    else if (main_cmd == SHOW)
    {
      handle_show();
    }
  }
}

void UCICommunicator::handle_uci()
{
  /// TODO: add option command in here to give engine options 
  std::cout << ID_NAME;
  std::cout << ID_AUTHOR;
  std::cout << UCIOK;
}

void UCICommunicator::handle_is_ready()
{
  std::cout << READYOK;
}

void UCICommunicator::handle_new_game()
{
  m_board->reset(); // probably not necessary
}

void UCICommunicator::handle_position(std::vector<std::string>& parsed_cmd, std::string& cmd)
{
  std::string fen;
  if (parsed_cmd[1] == STARTPOS)
  {
    fen = STARTFEN;
  }
  else
  {
    /* extract the input fen string */
    for (size_t i = 0; i < parsed_cmd.size(); i++)
    {
      if (i < 2)
        continue; /* ignore the "position fen" parts */

      if (i > 7)
      {
        fen.pop_back(); /* remove final space */
        break; /* everything past this is moves */
      }
      fen += parsed_cmd[i] + " ";
    }
  }

  /* find the moves at the end */
  std::vector<std::string> long_algebraic_moves;
  auto it = std::find(parsed_cmd.begin(), parsed_cmd.end(), "moves");
  while (it < parsed_cmd.end() - 1) /* making sure we don't include "moves" or dereference .end() */
  {
    it++;
    long_algebraic_moves.push_back(*it);
  }

  m_board->reset(fen);

  for (std::string algebraic_move : long_algebraic_moves)
  {
    Move move = m_move_gen.move_from_long_algebraic(algebraic_move);
    m_board->make_move(move);
  }
}

void UCICommunicator::handle_go(std::vector<std::string>& parsed_cmd, std::string& cmd)
{
  if (parsed_cmd.size() == 1 || parsed_cmd[1] != PERFT)
  {
    Move bestmove = m_searcher.find_best_move(1000);
    std::cout << "bestmove " << m_move_gen.move_to_long_algebraic(bestmove) << std::endl;
  } 
  else if (parsed_cmd[1] == PERFT)
  {
    if (parsed_cmd.size() < 3)
    {
      return;
    }
    int depth = std::stoi(parsed_cmd[2]);
    m_searcher.perft(depth);
  }
}

void UCICommunicator::handle_verify(std::vector<std::string>& parsed_cmd)
{
  int depth = parsed_cmd.size() > 1 ? std::stoi(parsed_cmd[1]) : 5; /* default */
  std::string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  std::string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
  std::string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  std::string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
  std::string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

  m_board->reset(test_pos_1);
  std::cout << "Test 1 total: " << m_searcher.num_nodes_bulk(depth) << std::endl;
  m_board->reset(test_pos_2);
  std::cout << "Test 2 total: " << m_searcher.num_nodes_bulk(depth) << std::endl;
  m_board->reset(test_pos_3);
  std::cout << "Test 3 total: " << m_searcher.num_nodes_bulk(depth) << std::endl;
  m_board->reset(test_pos_4);
  std::cout << "Test 4 total: " << m_searcher.num_nodes_bulk(depth) << std::endl;
  m_board->reset(test_pos_5);
  std::cout << "Test 5 total: " << m_searcher.num_nodes_bulk(depth) << std::endl;
  m_board->reset(test_pos_6);
  std::cout << "Test 6 total: " << m_searcher.num_nodes_bulk(depth) << std::endl;
}

void UCICommunicator::handle_quit()
{
  exit(0); /* quit successfully*/
}

void UCICommunicator::handle_show()
{
  std::cout << m_board->to_string();
}