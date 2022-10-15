#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "search.h"
#include "pieces.h"
#include "board.h"
#include "hashing.h"
#include "openings.h"
#include "attacks.h"
#include "hashing.h"
#include "tt.h"
#include "evaluation.h"
#include "debugging.h"
#include "moves.h"

/*
Plan: make us able to play a full game of chess in this format (passing in the moves and such)
 * need to make printing the board better
 * need to make a notation to move function (* if this doesn't already exist *)
 * I really need to figure out how I am going to go about printing and getting the principle variation...
 * Oh maybe its just the best move after each depth? (oh no its not that will change upon each iteration)
*/

int main() {
    luts = init_LUT(); // must do this first
    zobrist_table = init_zobrist();
    // opening_book = create_opening_book(); // uncomment if you need to update opening_book
    // generate_num_data(); // uncomment if you need to update opening_book
    opening_book = populate_opening_book();
    init_tt_table();
    init_eval_table();

    string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

    size_t depth;
    size_t total_nodes;
    clock_t tStart;
    clock_t tStop;
    double time_elapsed;

    char answer;
    string fen;
    int search_time;
    while(true) {
        cout << "Perft test or speed test or move test or quit? (p/s/m/q)" << endl;
        cin >> answer;
        if(answer == 'p') {
            cout << endl << "Enter depth: ";
            cin >> depth;

            decode_fen(test_pos_1);
            cout << "Test 1 at depth " << depth << endl;
            perft(depth);
            cout << endl;

            decode_fen(test_pos_2);
            cout << "Test 2 at depth " << depth << endl;
            perft(depth);
            cout << endl;

            decode_fen(test_pos_3);
            cout << "Test 3 at depth " << depth << endl;
            perft(depth);
            cout << endl;

            decode_fen(test_pos_4);
            cout << "Test 4 at depth " << depth << endl;
            perft(depth);
            cout << endl;

            decode_fen(test_pos_5);
            cout << "Test 5 at depth " << depth << endl;
            perft(depth);
            cout << endl;

            decode_fen(test_pos_6);
            cout << "Test 6 at depth " << depth << endl;
            perft(depth);
            cout << endl;
        }
        else if(answer == 's') {
            cout << endl << "Enter depth: ";
            cin >> depth;
            total_nodes = 0;
            tStart = clock();
            /* having to decode the fen in between will slow it down */
            /* rework this for that reason */
            decode_fen(test_pos_1);
            total_nodes += num_nodes_bulk(depth);
            decode_fen(test_pos_2);
            total_nodes += num_nodes_bulk(depth);
            decode_fen(test_pos_3);
            total_nodes += num_nodes_bulk(depth);
            decode_fen(test_pos_4);
            total_nodes += num_nodes_bulk(depth);
            decode_fen(test_pos_5);
            total_nodes += num_nodes_bulk(depth);
            decode_fen(test_pos_6);
            total_nodes += num_nodes_bulk(depth);
            tStop = clock();
            time_elapsed = (double)(tStop - tStart)/CLOCKS_PER_SEC;
            cout << "Total nodes: " << total_nodes << endl;
            cout << "Time elapsed: " << time_elapsed << endl;
            cout << "Nodes per second: " << ((double)total_nodes / time_elapsed) << endl << endl;
        }
        else if(answer == 'm') {
            cout << endl << "Enter fen to search: ";
            cin >> fen;
            cout << endl << "Enter time to search (in milliseconds): ";
            cin >> search_time;
            decode_fen(fen);
            cout << endl;
            print_squarewise(b.sq_board);
            cout << endl;
            move_t move = find_best_move(search_time);
            cout << endl << "Best move: ";
            cout << notation_from_move(move) << endl;
        }
        else if(answer == 'q')
            break;
    }
    free_tt_table();
    free_eval_table();
    return 0;
}