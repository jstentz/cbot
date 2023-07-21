#include <stdio.h>
#include <memory>
#include <iostream>
#include <chrono>

#include "include/search.h"
#include "include/board.h"
#include "include/utils.h"
#include "include/uci.h"

int main() 
{
  std::string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  std::string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
  std::string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  std::string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
  std::string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

  // Board::Ptr board = std::make_shared<Board>(test_pos_1);
  // Searcher searcher{board};
  // int d;
  // std::cin >> d;
  // uint64_t total = 0;
  // auto begin = std::chrono::high_resolution_clock::now();
  // total += searcher.perft(d);
  // board->reset(test_pos_2);
  // total += searcher.perft(d);
  // board->reset(test_pos_3);
  // total += searcher.perft(d);
  // board->reset(test_pos_4);
  // total += searcher.perft(d);
  // board->reset(test_pos_5);
  // total += searcher.perft(d);
  // board->reset(test_pos_6);
  // total += searcher.perft(d);
  // auto end = std::chrono::high_resolution_clock::now();
  // auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  // double seconds = nanoseconds * 1e-9;
  // std::cout << seconds << std::endl;
  // std::cout << ((double)total / seconds) << std::endl;


  Board::Ptr board = std::make_shared<Board>();
  Searcher searcher{board};
  MoveGenerator move_gen{board};

  board->reset(test_pos_1);
  std::cout << board->to_string();
  std::cout << move_gen.move_to_long_algebraic(searcher.find_best_move(1000)) << std::endl;

  // UCICommunicator uci;
  // uci.start_uci_communication();

  return 0;
}