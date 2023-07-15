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

/**
 * @brief Splits a string into a list of strings that are separated by the input delimiter.
 * 
 * @param s 
 * @param delim 
 * @return std::vector<std::string> 
 */
std::vector<std::string> split(const std::string &s, char delim);


/**
 * @brief Returns a random 64-bit number.
 * 
 * @return uint64_t 
 */
uint64_t rand64();

}