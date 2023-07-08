#pragma once

#include <string>

#include "include/pieces.h"
#include "board.h"

namespace utils
{
piece piece_from_fen_char(char c);

Board::Square sq_from_name(std::string name);
}