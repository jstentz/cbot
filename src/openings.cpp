#include "openings.h"
#include "hashing.h"
#include "board.h"
#include "moves.h"

#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <fstream>

using namespace std;

unordered_map<hash_val, vector<move_t>> create_opening_book() {
    string starting_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    board_t starting_pos = decode_fen(starting_FEN);
    hash_val h = zobrist_hash(&starting_pos);

    unordered_map<hash_val, vector<move_t>> opening_book;
    ifstream openings_file("assets/opening_book.pgn");
    string line;
    while(getline(openings_file, line)) {
        cout << line << endl;
    }

    
    // unordered_map<hash_val, int> book = *opening_book;
    // book.insert(pair<hash_val, int> )

    // use find and insert to add to the move list
    return opening_book;
}

// int main() {
//     // fill_opening_book();
//     int x;
//     cin >> x;
//     return 0;
// }