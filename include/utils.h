#pragma once

#include <string>

#include "include/pieces.h"
#include "include/constants.h"

namespace utils
{
piece piece_from_fen_char(char c);

int sq_from_name(std::string name);

inline static int file(int sq);
inline static int rank(int sq);
inline static int diag(int sq);
inline static int anti_diag(int sq);
}