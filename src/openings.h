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

#include "moves.h"
#include "hashing.h"

#include <unordered_map>
#include <vector>

/**
 * @brief Splits a string into a list of strings that are separated by the input delimiter.
 * 
 * @param s 
 * @param delim 
 * @return std::vector<std::string> 
 */
std::vector<std::string> split(const std::string &s, char delim);

/**
 * @brief Create a opening book object
 * 
 * @return unordered_map<hash_val, vector<move_t>> 
 */
unordered_map<hash_val, vector<move_t>> create_opening_book();

/**
 * @brief Searches the opening book for the current board state. Returns a move if
 * one exists for that position.
 * 
 * @return move_t 
 */
move_t get_opening_move();

extern unordered_map<hash_val, vector<move_t>> opening_book;

/**
 * @brief Converts the opening book to binary for faster reading.
 * 
 */
void generate_num_data();

/**
 * @brief Reads from the file to fill the usable opening book data structure.
 * 
 * @return unordered_map<hash_val, vector<move_t>> 
 */
unordered_map<hash_val, vector<move_t>> populate_opening_book();