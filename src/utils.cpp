#include <iostream>

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

Board::Square utils::sq_from_name(std::string name)
{
  if (name.size() != 2)
  {
    std::cerr << "Invalid square name!" << std::endl;
    exit(1);
  }

  int file = constants::FILES.find(name[0]);
  int rank = constants::RANKS.find(name[1]);

  return (Board::Square)(rank * 8 + file); 
}