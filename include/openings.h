/**
 * @file openings.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief Outlines structures and functions for playing moves in the 
 * opening phase of the game.
 * @version 0.1
 * @date 2022-06-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "include/move.h"
#include "include/hashing.h"

#include <unordered_map>
#include <vector>

/// @brief this setup doesn't really make sense, I should redo this and make it faster
class OpeningBook
{
  OpeningBook() {}
  ~OpeningBook() {} 

  /// @brief populates the opening book from binary text, can take a little bit of time
  void initialize();

  Move get_opening_move(Board::Ptr board);

  /// @brief creates the opening book from the text file of lines 
  // std::unordered_map<hash_val, std::vector<Move>> create_opening_book() const;

  /// @brief converts the book to a file of numbers instead of moves
  // void generate_binary_file();

private:
  std::unordered_map<hash_val, std::vector<Move>> m_opening_book;
};