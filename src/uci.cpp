#include <iostream>
#include <thread>
#include <string>
#include <bits/stdc++.h> 

#include "include/uci.h"
#include "include/search.h"
#include "include/board.h"
#include "include/move.h"
#include "include/tt.h"
#include "include/evaluation.h"

std::vector<std::string> uci::split_cmd(std::string& cmd)
{
  std::vector<std::string> str_list;
  std::string temp_str;
  std::stringstream ss{cmd};
  while (std::getline(ss, temp_str, ' '))
  {
    str_list.push_back(temp_str);
  }
  return str_list;
}


/// TODO: make it so commands at the wrong time don't work
void uci::start_uci_communication()
{
  std::string cmd;
  std::vector<std::string> cmd_list;
  std::string main_cmd;

  /* wait for the uci cmd */
  while (std::getline(std::cin, cmd))
  {
    /* extract command parts */
    cmd_list = uci::split_cmd(cmd);
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
    else if (main_cmd == QUIT)
    {
      handle_quit(); 
    }
    else if (main_cmd == VERIFY)
    {
      handle_verify(cmd_list);
    }
  }
}

void uci::handle_uci()
{
  /// TODO: add option command in here to give engine options 
  std::cout << ID_NAME;
  std::cout << ID_AUTHOR;
  std::cout << UCIOK;
}

void uci::handle_is_ready()
{
  std::cout << READYOK;
}

void uci::handle_new_game()
{
  /* do nothing for now */
}

void uci::handle_position(std::vector<std::string>& parsed_cmd, std::string& cmd)
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

  /* use this info to get to this position on the board */
  decode_fen(fen);

  for (std::string algebraic_move : long_algebraic_moves)
  {
    move_t move = long_algebraic_to_move(algebraic_move);
    make_move(move);
  }
}

void uci::handle_verify(std::vector<std::string>& parsed_cmd)
{
  int depth = parsed_cmd.size() > 1 ? std::stoi(parsed_cmd[1]) : 5; /* default */
  std::string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
  std::string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
  std::string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  std::string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
  std::string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

  decode_fen(test_pos_1);
  std::cout << "Test 1 total: " << num_nodes_bulk(depth) << std::endl;
  decode_fen(test_pos_2);
  std::cout << "Test 2 total: " << num_nodes_bulk(depth) << std::endl;
  decode_fen(test_pos_3);
  std::cout << "Test 3 total: " << num_nodes_bulk(depth) << std::endl;
  decode_fen(test_pos_4);
  std::cout << "Test 4 total: " << num_nodes_bulk(depth) << std::endl;
  decode_fen(test_pos_5);
  std::cout << "Test 5 total: " << num_nodes_bulk(depth) << std::endl;
  decode_fen(test_pos_6);
  std::cout << "Test 6 total: " << num_nodes_bulk(depth) << std::endl;
}

void uci::handle_quit()
{
  free_tt_table();
  free_eval_table();
  exit(0); /* quit successfully*/
}