#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <string>
#include <sstream>

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
      break;
    default: 
      std::cerr << "Invalid piece name!" << std::endl;
      exit(1);
  };
  return color | type;
}

int utils::sq_from_name(std::string name)
{
  if (name == "-")
  {
    return constants::NONE;
  }
  else if (name.size() != 2)
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

std::vector<std::string> utils::split(std::string& cmd, char delim)
{
  std::vector<std::string> str_list;
  std::string temp_str;
  std::stringstream ss{cmd};
  while (std::getline(ss, temp_str, delim))
  {
    str_list.push_back(temp_str);
  }
  return str_list;
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

bool utils::is_mate_score(int score) 
{
  return (score > (INT_MAX - 100)) || (score < ((INT_MIN + 1) + 100));
}

int utils::moves_until_mate(int mate_score) 
{
  mate_score = abs(mate_score);
  return (INT_MAX - mate_score - 1) / 2;
}


int utils::cmd(int sq) 
{
  return constants::center_manhattan_distance_arr[sq];
}

int utils::md(int sq1, int sq2) 
{
  int f1 = file(sq1);
  int f2 = file(sq2);
  int r1 = rank(sq1);
  int r2 = rank(sq2);
  return abs(r2 - r1) + abs(f2 - f1);
}