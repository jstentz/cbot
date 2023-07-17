#pragma once

#include <string>
#include <vector>

#include "include/pieces.h"
#include "include/constants.h"
#include "include/bitboard.h"


namespace utils
{
piece piece_from_fen_char(char c);

int sq_from_name(std::string name);

inline int file(int sq)
{
  return sq & 7;
}

inline int rank(int sq)
{
  return sq >> 3;
}

inline int diag(int sq)
{
  return 7 + rank(sq) - file(sq);
}

inline int anti_diag(int sq)
{
  return rank(sq) + file(sq);
}

inline int index_from_pc(piece pc)
{
  return pc - 2; /// TODO: make a comment on why this works
}

/**
 * @brief Splits a string into a list of strings that are separated by the input delimiter.
 * 
 * @param s 
 * @param delim 
 * @return std::vector<std::string> 
 */
std::vector<std::string> split(std::string& cmd, char delim);


/**
 * @brief Returns a random 64-bit number.
 * 
 * @return uint64_t 
 */
uint64_t rand64();

/**
 * @brief Given a score, returns true if it is a mating score and false otherwise.
 * 
 * @param score 
 * @return true 
 * @return false 
 */
bool is_mate_score(int score);

/**
 * @brief Given a mating score, returns the moves until checkmate
 * 
 * @param mate_score 
 * @return moves until mate 
 */
int moves_until_mate(int mate_score);

int cmd(int sq);
int md(int sq1, int sq2);

void print_bitboard(bitboard b);

}