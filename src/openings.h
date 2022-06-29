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

unordered_map<hash_val, vector<move_t>> create_opening_book();

move_t get_opening_move();

extern unordered_map<hash_val, vector<move_t>> opening_book;

void generate_num_data();

unordered_map<hash_val, vector<move_t>> populate_opening_book();