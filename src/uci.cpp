#include <iostream>
#include <thread>
#include <string>
#include <bits/stdc++.h> 

#include "include/uci.h"
#include "include/search.h"

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
      handle_position(cmd_list);
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

void uci::handle_position(std::vector<std::string> cmd_list)
{
  /* need a better way to extract the fen here */
  std::string fen;
  if (cmd_list[1] == STARTPOS)
  {
    fen = starting_fen;
  }
  else if (cmd_list[1] == FEN)
  {
    for (std::string &tmp : cmd_list)
    {
      std::cout << tmp << std::endl;
    }
  }
}