#include "../include/openings.h"
#include "../include/hashing.h"
#include "../include/board.h"
#include "../include/moves.h"

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

using namespace std;

template <typename Out>
void splitHelp(const std::string &s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  splitHelp(s, delim, std::back_inserter(elems));
  return elems;
}

unordered_map<hash_val, vector<move_t>> create_opening_book() {
  srand(clock());
  string starting_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  decode_fen(starting_FEN);

  // board_t board;

  hash_val h;

  unordered_map<hash_val, vector<move_t>> opening_book_local;

  ifstream openings_file("assets/opening_book_lines.pgn");
  string line;
  move_t move;
  unordered_map<hash_val, vector<move_t>>::iterator got_board;
  vector<string> split_line;
  string notation;
  while(getline(openings_file, line)) {
    split_line = split(line, ' ');
    for (int i = 0; i < split_line.size(); i++) {
      notation = split_line[i];
      if(notation == "") continue;
      h = zobrist_hash();
      move = move_from_notation(notation);
      got_board = opening_book_local.find(h);
      if(got_board == opening_book_local.end()) { // board is not in the map
        vector<move_t> empty;
        opening_book_local.insert(pair<hash_val, vector<move_t>> (h, empty));
      }
      else { // board is in the map
        got_board->second.push_back(move);
      }
      if(i == (split_line.size() - 1)) { // don't need to make the final move
        break;
      }
      make_move(move);
    }
    decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  }

  openings_file.close();
  return opening_book_local;
}

unordered_map<hash_val, vector<move_t>> opening_book;

void generate_num_data() {
  ofstream outfile;
  outfile.open("assets/opening_book.pgn");
  vector<move_t> moves;
  unordered_map<hash_val, vector<move_t>>::iterator it;
  for(it = opening_book.begin(); it != opening_book.end(); it++) {
    moves = it->second;
    if(moves.size() == 0) continue; // don't care about positions we don't know the theory for
    outfile << it->first << " ";

    moves = it->second;
    for (move_t move : moves) {
      outfile << move << " ";
    }
    outfile << "\n";
  }
  outfile.close();
  return;
}

unordered_map<hash_val, vector<move_t>> populate_opening_book() {
  srand(clock());
  ifstream openings_file("assets/opening_book.pgn");
  unordered_map<hash_val, vector<move_t>> opening_book_local;
  string line;
  vector<string> split_line;
  vector<move_t> moves;
  while(getline(openings_file, line)) {
    split_line = split(line, ' ');
    hash_val h = stoull(split_line[0]);
    // cout << h << endl;
    moves.clear();
    for(int i = 1; i < split_line.size(); i++) {
      moves.push_back(stoi(split_line[i]));
    }
    opening_book_local.insert(pair<hash_val, vector<move_t>> (h, moves));
  }
  return opening_book_local;
}


move_t get_opening_move() {
  hash_val h = b.board_hash;
  unordered_map<hash_val, vector<move_t>>::iterator got_board = opening_book.find(h);
  if(got_board == opening_book.end()) {
    return NO_MOVE; // position not found
  }
  unsigned int r = rand64() % got_board->second.size();

  if(got_board->second.size() == 0) {
    cout << "no known moves from this position!" << endl;
    int x;
    cin >> x;
  }
  
  return got_board->second[r];
}
