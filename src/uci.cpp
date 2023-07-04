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

void uci::start_uci_communication()
{
  std::string cmd;
  std::vector<std::string> cmd_list;
  while (std::getline(std::cin, cmd))
  {
    cmd_list = uci::split_cmd(cmd);
    for (std::string& tmp : cmd_list)
    { 
      std::cout << tmp << std::endl;
    }
  }
}