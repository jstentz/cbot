#include "search.h"
#include "bitboard.h"
#include "board.h"
#include "moves.h"
#include "evaluation.h"

#include <stddef.h>
#include <stack>
#include <vector>
#include <time.h>

using namespace std;

size_t positions_searched = 0; // speed test purposes

uint64_t num_nodes_bulk(stack<board_t *> *board, size_t depth) {
    board_t *curr_board = (*board).top();
    board_t *next_board;
    vector<move_t> moves;
    generate_moves(curr_board, &moves);
    if(depth == 1) {
        return moves.size();
    }
    else if(depth == 0) {
        return 1;
    }

    uint64_t total_moves = 0;
    for(move_t move : moves) {
        next_board = make_move(curr_board, &move);
        (*board).push(next_board);
        total_moves += num_nodes_bulk(board, depth - 1); 
        unmake_move(board);
    }
    return total_moves;
}

uint64_t num_nodes(stack<board_t *> *board, size_t depth) {
    vector<move_t> moves;
    board_t *curr_board = (*board).top();
    board_t *next_board;
    if(depth == 0) {
        return 1;
    }

    uint64_t total_moves = 0;
    generate_moves(curr_board, &moves);
    for(move_t move : moves) {
        next_board = make_move(curr_board, &move);
        (*board).push(next_board);
        total_moves += num_nodes(board, depth - 1); 
        unmake_move(board);
    }
    return total_moves;
}

uint64_t perft(board_t *board, size_t depth) {
    vector<move_t> moves;
    generate_moves(board, &moves);
    stack<board_t *> board_stack;
    board_stack.push(board);
    uint64_t total_nodes = 0; // this can overflow, should change
    uint64_t nodes_from_move = 0;
    for(move_t move : moves) {
        board_stack.push(make_move(board, &move));
        cout << notation_from_move(move, moves, board) << ": ";
        nodes_from_move = num_nodes_bulk(&board_stack, depth - 1);
        total_nodes += nodes_from_move;
        cout << nodes_from_move << endl;
        unmake_move(&board_stack);
    }
    cout << "Nodes searched: " << total_nodes << endl;
    return total_nodes;
}

// go back through and comment this more to understand it
int qsearch(stack<board_t *> *board_stack, int alpha, int beta) {
    vector<move_t> captures;
    board_t *curr_board = (*board_stack).top();
    board_t *next_board;

    int stand_pat = evaluate(curr_board); // fall back evaluation
    if(stand_pat >= beta) {positions_searched++; return beta;}
    if(alpha < stand_pat) alpha = stand_pat;

    generate_moves(curr_board, &captures, true); // true flag generates only captures
    order_moves(&captures);
    for (move_t capture : captures) {
        next_board = make_move(curr_board, &capture);
        (*board_stack).push(next_board);
        int evaluation = -qsearch(board_stack, -beta, -alpha);
        unmake_move(board_stack);

        if(evaluation >= beta) {positions_searched++; return beta;}
        if(evaluation > alpha) alpha = evaluation;
    }
    positions_searched++;
    return alpha;
}

int search(stack<board_t *> *board_stack, size_t depth, int alpha, int beta) {
    vector<move_t> moves;
   
    if(depth == 0) {
        return qsearch(board_stack, alpha, beta);
    }

    board_t *curr_board = (*board_stack).top();
    board_t *next_board;
    
    generate_moves(curr_board, &moves);
    order_moves(&moves);
    if(moves.size() == 0) {
        if(checking_pieces(curr_board) != 0) {
            return INT_MIN + 1 + (*board_stack).size(); // the deeper in the search we are, the less good the checkmate is
        }
        return 0;
    }

    int best_eval = INT_MIN + 1;

    for(move_t move : moves) {
        next_board = make_move(curr_board, &move);
        (*board_stack).push(next_board);
        int evaluation = -search(board_stack, depth - 1, -beta, -alpha);
        unmake_move(board_stack);
        if(evaluation > best_eval) best_eval = evaluation;
        if(best_eval > alpha) alpha = best_eval;
        if(alpha >= beta) break;
    }
    return best_eval;
}

move_t find_best_move(board_t *board) {
    clock_t tStart;
    clock_t tStop;
    size_t depth = 5; // how do I know how deep to search?
    stack<board_t *> board_stack;
    board_stack.push(board);
    vector<move_t> moves;
    generate_moves(board, &moves);
    order_moves(&moves);
    move_t best_move = moves[0]; // this will crash it at some point
    int best_eval = INT_MIN + 1;
    int alpha = INT_MIN + 1;
    int beta = INT_MAX;
    int move_num = 0;
    int count = 0;
    tStart = clock();
    for(move_t move : moves) {
        board_stack.push(make_move(board, &move));
        int eval = -search(&board_stack, depth - 1, -beta, -alpha); // now its black's move

        if(eval > best_eval) {
            best_eval = eval;
            best_move = move;
            move_num = count;
            if(best_eval > alpha) alpha = best_eval; // not sure if this is right
        }
        unmake_move(&board_stack);
        count++;
    }
    tStop = clock();
    // more speed debugging stuff
    cout << "Positions searched: " << positions_searched << endl;
    cout << "This move was in position " << move_num << " out of " << count << endl;
    double time_elapsed = (double)(tStop - tStart)/CLOCKS_PER_SEC;
    cout << "Time elapsed: " << time_elapsed << endl;
    cout << "Nodes per second: " << ((double)positions_searched / time_elapsed) << endl;
    cout << "Material score: " << board->material_score << endl;
    positions_searched = 0;
    return best_move;
}

// int main() {
//     string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//     string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//     string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
//     string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
//     string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
//     string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
//     string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

//     board_t *board_1 = decode_fen(test_pos_1);
//     board_t *board_2 = decode_fen(test_pos_2);
//     board_t *board_3 = decode_fen(test_pos_3);
//     board_t *board_4 = decode_fen(test_pos_4);
//     board_t *board_5 = decode_fen(test_pos_5);
//     board_t *board_6 = decode_fen(test_pos_6);

//     // string test_pos = "3R4/3Q4/8/8/2k5/4K3/8/8 b - - 0 1";
//     // board_t *board = decode_fen(test_pos);
//     // vector<move_t> moves;
//     // generate_moves(board, &moves);
//     // cout << notation_from_move(find_best_move(board, 5), moves, board) << endl;

//     size_t depth;
//     size_t total_nodes;
//     clock_t tStart;
//     clock_t tStop;
//     double time_elapsed;

//     stack<board_t *> board_1_stack;
//     stack<board_t *> board_2_stack;
//     stack<board_t *> board_3_stack;
//     stack<board_t *> board_4_stack;
//     stack<board_t *> board_5_stack;
//     stack<board_t *> board_6_stack;
//     board_1_stack.push(board_1);
//     board_2_stack.push(board_2);
//     board_3_stack.push(board_3);
//     board_4_stack.push(board_4);
//     board_5_stack.push(board_5);
//     board_6_stack.push(board_6);
//     char answer;
//     while(true) {
//         cout << "Perft test or speed test? (p/s)" << endl;
//         cin >> answer;
//         if(answer == 'p') {
//             cout << endl << "Enter depth: ";
//             cin >> depth;

//             cout << "Test 1 at depth " << depth << endl;
//             perft(board_1, depth);
//             cout << endl;

//             cout << "Test 2 at depth " << depth << endl;
//             perft(board_2, depth);
//             cout << endl;

//             cout << "Test 3 at depth " << depth << endl;
//             perft(board_3, depth);
//             cout << endl;

//             cout << "Test 4 at depth " << depth << endl;
//             perft(board_4, depth);
//             cout << endl;

//             cout << "Test 5 at depth " << depth << endl;
//             perft(board_5, depth);
//             cout << endl;

//             cout << "Test 6 at depth " << depth << endl;
//             perft(board_6, depth);
//             cout << endl;
//         }
//         else if(answer == 's') {
//             cout << endl << "Enter depth: ";
//             cin >> depth;
//             total_nodes = 0;
//             tStart = clock();
//             total_nodes += num_nodes_bulk(&board_1_stack, depth);
//             total_nodes += num_nodes_bulk(&board_2_stack, depth);
//             total_nodes += num_nodes_bulk(&board_3_stack, depth);
//             total_nodes += num_nodes_bulk(&board_4_stack, depth);
//             total_nodes += num_nodes_bulk(&board_5_stack, depth);
//             total_nodes += num_nodes_bulk(&board_6_stack, depth);
//             tStop = clock();
//             time_elapsed = (double)(tStop - tStart)/CLOCKS_PER_SEC;
//             cout << "Total nodes: " << total_nodes << endl;
//             cout << "Time elapsed: " << time_elapsed << endl;
//             cout << "Nodes per second: " << ((double)total_nodes / time_elapsed) << endl << endl;
//         }
//     }

//     free(board_1);
//     free(board_2);
//     free(board_3);
//     free(board_4);
//     free(board_5);
//     free(board_6);
//     return 0;
// }

/*
    Current speed: ~ 47.5 million nps
    After moving luts to global variable, ~ 48 million nps
    After changing the place and rem functions, ~ 48.3 million nps
    After making move a pointer, ~ 48.4 million nps
    on a bigger sample size (depth 5) its ~ 52.5 million nps

    pretty much can no longer test this since there we will slow it down with
    incremental evaluation of the board

    currently 2.5 million nodes evaluated per second

    maybe have the opening book be like a hashtable that has the position and 
    the move to be played in that position

    currently en passant is not in the tar_piece in the move... maybe change that
    but it will affect other things like make_move

    don't like how I check for captures only (just use if statement)
    make is_attacked return the piece that is attacking that square (DONT DO THAT)
    make least_valued_attacker return the least valued piece attacking a square
    ^ used for move ordering purposes

    most programs consider a single repetition to be a draw

    need to add draw for insufficient material
    tomorrow need to do better eval and better move ordering
    have make_move function update hash value
*/