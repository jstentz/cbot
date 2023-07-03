#include <iostream>
#include <thread>

#include "include/uci.h"
#include "include/search.h"

void uci::start_uci_communication()
{
  std::string input;
  while (std::getline(std::cin, input))
  {
    std::cout << input << std::endl; 
  }
}