#include "include/openings.h"
#include "include/hashing.h"
#include "include/board.h"
#include "include/move.h"
#include "include/utils.h"
#include "include/constants.h"

#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>
#include <stack>
#include <random>

OpeningBook::OpeningBook()
{
  m_opening_book.clear();
  srand(clock());
  std::ifstream openings_file("assets/opening_book.pgn");
  std::string line;
  std::vector<std::string> split_line;
  std::vector<Move> moves;
  while(getline(openings_file, line)) {
    split_line = utils::split(line, ' ');
    uint64_t h = stoull(split_line[0]);
    // std::cout << h << endl;
    moves.clear();
    for(int i = 1; i < split_line.size(); i++) {
      moves.emplace_back(stoi(split_line[i]));
    }
    m_opening_book.insert(std::pair<uint64_t, std::vector<Move>>{h, moves});
  }
}

Move OpeningBook::get_opening_move(Board::Ptr board)
{
  uint64_t h = board->get_hash();
  std::cout << h << std::endl;
  std::unordered_map<uint64_t, std::vector<Move>>::iterator got_board = m_opening_book.find(h);
  if(got_board == m_opening_book.end()) 
  {
    std::cout << "here!" << std::endl;
    return Move::NO_MOVE; // position not found
  }
  unsigned int r = utils::rand64() % got_board->second.size();

  if(got_board->second.size() == 0) 
  {
    std::cerr << "No known moves from this position!" << std::endl;
  }
  return got_board->second[r];
}


/// TODO: these functions below are kinda useless... ignoring for now
// std::unordered_map<uint64_t, std::vector<Move>> OpeningBook::create_opening_book() const;


// std::unordered_map<uint64_t, std::vector<move_t>> create_opening_book() {
//   srand(clock());
//   decode_fen(constants::STARTFEN);

//   // board_t board;

//   uint64_t h;

//   std::unordered_map<uint64_t, std::vector<move_t>> opening_book_local;

//   std::ifstream openings_file("assets/opening_book_lines.pgn");
//   std::string line;
//   move_t move;
//   std::unordered_map<uint64_t, std::vector<move_t>>::iterator got_board;
//   std::vector<std::string> split_line;
//   std::string notation;
//   while(getline(openings_file, line)) {
//     split_line = split(line, ' ');
//     for (int i = 0; i < split_line.size(); i++) {
//       notation = split_line[i];
//       if(notation == "") continue;
//       h = zobrist_hash();
//       move = move_from_notation(notation);
//       got_board = opening_book_local.find(h);
//       if(got_board == opening_book_local.end()) { // board is not in the map
//         std::vector<move_t> empty;
//         opening_book_local.insert(std::pair<uint64_t, std::vector<move_t>> (h, empty));
//       }
//       else { // board is in the map
//         got_board->second.push_back(move);
//       }
//       if(i == (split_line.size() - 1)) { // don't need to make the final move
//         break;
//       }
//       make_move(move);
//     }
//     decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
//   }

//   openings_file.close();
//   return opening_book_local;
// }

// std::unordered_map<uint64_t, std::vector<move_t>> opening_book;

// void generate_num_data() {
//   std::ofstream outfile;
//   outfile.open("assets/opening_book.pgn");
//   std::vector<move_t> moves;
//   std::unordered_map<uint64_t, std::vector<move_t>>::iterator it;
//   for(it = opening_book.begin(); it != opening_book.end(); it++) {
//     moves = it->second;
//     if(moves.size() == 0) continue; // don't care about positions we don't know the theory for
//     outfile << it->first << " ";

//     moves = it->second;
//     for (move_t move : moves) {
//       outfile << move << " ";
//     }
//     outfile << "\n";
//   }
//   outfile.close();
//   return;
// }


// move_t get_opening_move() {
  
// }
