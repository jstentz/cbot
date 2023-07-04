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
      handle_position(cmd_list, cmd);
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

void uci::handle_position(std::vector<std::string> parsed_cmd, std::string& cmd)
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

  /// TODO: call board_from_fen function
  /// TODO: loop over algebraic moves and make them on the board
  /// TODO: make the board_from_fen better (sscanf)
}

/* create a board from position and moves function elsewhere and call that */
/* I should really be using std::sscanf in my fen to position function that would make things so much easier holy shit */
/* position fen main turn castle en half full */