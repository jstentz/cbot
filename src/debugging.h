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

/**
 * @brief Prints a bitboard in a nice format.
 * 
 * @param b Bitboard to print
 */
void print_bitboard(bitboard b);

/**
 * @brief Prints a squarewise board in a nice format.
 * 
 * @param b Board to print
 */
void print_squarewise(piece sqs[64]);

/**
 * @brief Prints all aspects of the board state in a nice position.
 * 
 * @param b Board to print
 */
void print_board(board_t *board);