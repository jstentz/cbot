#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdint>

#include "include/utils.h"
#include "include/pieces.h"

piece utils::piece_from_fen_char(char c)
{
  piece color = islower(c) ? BLACK : WHITE; 
  piece type;
  switch (tolower(c))
  {
    case 'p':
      type = PAWN;
      break;
    case 'n':
      type = KNIGHT;
      break;
    case 'b':
      type = BISHOP;
      break;
    case 'r':
      type = ROOK;
      break;
    case 'q':
      type = QUEEN;
      break;
    case 'k':
      type = KING;
    default: 
      std::cerr << "Invalid piece name!" << std::endl;
      exit(1);
  };
  return color | type;
}

int utils::sq_from_name(std::string name)
{
  if (name.size() != 2)
  {
    std::cerr << "Invalid square name!" << std::endl;
    exit(1);
  }

  int file = constants::FILES.find(name[0]);
  int rank = constants::RANKS.find(name[1]);

  return rank * 8 + file; 
}

int utils::file(int sq)
{
  return sq & 7;
}

int utils::rank(int sq)
{
  return sq >> 3;
}

int utils::diag(int sq)
{
  return 7 + rank(sq) - file(sq);
}

int utils::anti_diag(int sq)
{
  return rank(sq) + file(sq);
}

int utils::index_from_pc(piece pc)
{
  return pc - 2; /// TODO: make a comment on why this works
}

template <typename Out>
void splitHelp(const std::string &s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = item;
  }
}

std::vector<std::string> utils::split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  splitHelp(s, delim, std::back_inserter(elems));
  return elems;
}

uint64_t utils::rand64() 
{
  uint64_t r = 0;
  for (int i = 0; i < 64; i++) 
  {
    if (rand() % 2 == 0) 
    {
      r = r | 0x1;
    }
    r = r << 1;
  }
  return r;
}