#pragma once

#include <string>

#include "include/pieces.h"
#include "include/constants.h"

namespace utils
{
piece piece_from_fen_char(char c);

int sq_from_name(std::string name);

int file(int sq);
int rank(int sq);
int diag(int sq);
int anti_diag(int sq);

int index_from_pc(piece pc);

}