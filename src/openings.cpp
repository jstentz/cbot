#include "openings.h"
#include "hashing.h"
#include "board.h"
#include "moves.h"

#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>

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
    string starting_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    board_t starting_pos = decode_fen(starting_FEN);
    hash_val h = zobrist_hash(&starting_pos);

    unordered_map<hash_val, vector<move_t>> opening_book;
    vector<move_t> empty;
    opening_book.insert(pair<hash_val, vector<move_t>> (h, empty));

    ifstream openings_file("assets/opening_book.pgn");
    string line;
    move_t mv;
    // while(getline(openings_file, line)) {
    //     opening_book.find()
    //     for (string move : split(line, ' ')) {
            
    //     }
    // }

    // consider using a hash_set for the moves and hashing the moves
    
    // unordered_map<hash_val, int> book = *opening_book;
    // book.insert(pair<hash_val, int> )

    // use find and insert to add to the move list

    // oh shit make the move a number and then we can use a set and put the 
    // moves in a set

    // check for set containment with structs
    // do this https://stackoverflow.com/questions/5865303/how-can-i-make-find-work-with-a-set-of-structs
    // https://stackoverflow.com/questions/3052788/how-to-select-a-random-element-in-stdset
    return opening_book;
}

// int main() {
//     // fill_opening_book();
//     int x;
//     cin >> x;
//     return 0;
// }