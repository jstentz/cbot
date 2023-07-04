#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "include/search.h"
#include "include/pieces.h"
#include "include/board.h"
#include "include/hashing.h"
#include "include/openings.h"
#include "include/attacks.h"
#include "include/hashing.h"
#include "include/tt.h"
#include "include/evaluation.h"
#include "include/debugging.h"
#include "include/moves.h"
#include "include/uci.h"

int main() 
{
  /// TODO: boot the engine here 
  luts = init_LUT(); // must do this first
  zobrist_table = init_zobrist();
  // opening_book = create_opening_book(); // uncomment if you need to update opening_book
  // generate_num_data(); // uncomment if you need to update opening_book
  opening_book = populate_opening_book();
  init_tt_table();
  init_eval_table();
  /* begin UCI listening */
  uci::start_uci_communication();
  return 0;
}

// int main() {
//   luts = init_LUT(); // must do this first
//   zobrist_table = init_zobrist();
//   // opening_book = create_opening_book(); // uncomment if you need to update opening_book
//   // generate_num_data(); // uncomment if you need to update opening_book
//   opening_book = populate_opening_book();
//   init_tt_table();
//   init_eval_table();

  // std::string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  // std::string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  // std::string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
  // std::string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
  // std::string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  // std::string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
  // std::string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

  // size_t depth;
  // size_t total_nodes;
  // clock_t tStart;
  // clock_t tStop;
  // double time_elapsed;

  // char answer;
  // std::string fen;
  // int search_time;
  // while(true) {
  //   std::cout << "Perft test or speed test or move test or quit? (p/s/m/q)" << std::endl;
  //   std::cin >> answer;
  //   if(answer == 'p') {
  //     std::cout << std::endl << "Enter depth: ";
  //     std::cin >> depth;

  //     decode_fen(test_pos_1);
  //     std::cout << "Test 1 at depth " << depth << std::endl;
  //     perft(depth);
  //     std::cout << std::endl;

  //     decode_fen(test_pos_2);
  //     std::cout << "Test 2 at depth " << depth << std::endl;
  //     perft(depth);
  //     std::cout << std::endl;

  //     decode_fen(test_pos_3);
  //     std::cout << "Test 3 at depth " << depth << std::endl;
  //     perft(depth);
  //     std::cout << std::endl;

  //     decode_fen(test_pos_4);
  //     std::cout << "Test 4 at depth " << depth << std::endl;
  //     perft(depth);
  //     std::cout << std::endl;

  //     decode_fen(test_pos_5);
  //     std::cout << "Test 5 at depth " << depth << std::endl;
  //     perft(depth);
  //     std::cout << std::endl;

  //     decode_fen(test_pos_6);
  //     std::cout << "Test 6 at depth " << depth << std::endl;
  //     perft(depth);
  //     std::cout << std::endl;
//     }
//     else if(answer == 's') {
//       std::cout << std::endl << "Enter depth: ";
//       std::cin >> depth;
//       total_nodes = 0;
//       tStart = clock();
//       /* having to decode the fen in between will slow it down */
//       /* rework this for that reason */
//       decode_fen(test_pos_1);
//       total_nodes += num_nodes_bulk(depth);
//       decode_fen(test_pos_2);
//       total_nodes += num_nodes_bulk(depth);
//       decode_fen(test_pos_3);
//       total_nodes += num_nodes_bulk(depth);
//       decode_fen(test_pos_4);
//       total_nodes += num_nodes_bulk(depth);
//       decode_fen(test_pos_5);
//       total_nodes += num_nodes_bulk(depth);
//       decode_fen(test_pos_6);
//       total_nodes += num_nodes_bulk(depth);
//       tStop = clock();
//       time_elapsed = (double)(tStop - tStart)/CLOCKS_PER_SEC;
//       std::cout << "Total nodes: " << total_nodes << std::endl;
//       std::cout << "Time elapsed: " << time_elapsed << std::endl;
//       std::cout << "Nodes per second: " << ((double)total_nodes / time_elapsed) << std::endl << std::endl;
//     }
//     else if(answer == 'q')
//       break;
//   }
//   free_tt_table();
//   free_eval_table();
//   return 0;
// }