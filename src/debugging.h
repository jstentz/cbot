/**
 * @file debugging.h
 * @author Jason Stentz (jstentz@andrew.cmu.edu)
 * @brief 
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "bitboard.h"
#include "pieces.h"
#include "board.h"

void print_bitboard(bitboard b);

void print_squarewise(piece sqs[64]);

void print_board(board_t *board);