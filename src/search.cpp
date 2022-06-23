#include "search.h"
#include "bitboard.h"
#include "board.h"
#include "moves.h"
#include "evaluation.h"
#include "hashing.h"
#include "openings.h"
#include "debugging.h"
#include "attacks.h"
#include "tt.h"

#include <stddef.h>
#include <stack>
#include <vector>
#include <time.h>
#include <unordered_set>
#include <algorithm>

using namespace std;

size_t positions_searched = 0; // speed test purposes

uint64_t num_nodes_bulk(stack<board_t> *board_stack, size_t depth) {
    vector<move_t> moves;
    generate_moves(&(*board_stack).top(), &moves);
    if(depth == 1) {
        return moves.size();
    }
    else if(depth == 0) {
        return 1;
    }

    uint64_t total_moves = 0;
    for(move_t move : moves) {
        make_move(board_stack, move);
        total_moves += num_nodes_bulk(board_stack, depth - 1); 
        unmake_move(board_stack);
    }
    return total_moves;
}

uint64_t num_nodes(stack<board_t> *board_stack, size_t depth) {
    vector<move_t> moves;
    if(depth == 0) {
        return 1;
    }

    uint64_t total_moves = 0;
    generate_moves(&(*board_stack).top(), &moves);
    for(move_t move : moves) {
        make_move(board_stack, move);
        total_moves += num_nodes(board_stack, depth - 1); 
        unmake_move(board_stack);
    }
    return total_moves;
}

uint64_t perft(stack<board_t> *board_stack, size_t depth) {
    vector<move_t> moves;
    board_t *curr_board = &(*board_stack).top();
    generate_moves(curr_board, &moves);
    uint64_t total_nodes = 0; // this can overflow, should change
    uint64_t nodes_from_move = 0;
    for(move_t move : moves) {
        make_move(board_stack, move);
        cout << notation_from_move(move, moves, curr_board) << ": ";
        nodes_from_move = num_nodes_bulk(board_stack, depth - 1);
        total_nodes += nodes_from_move;
        cout << nodes_from_move << endl;
        unmake_move(board_stack);
    }
    cout << "Nodes searched: " << total_nodes << endl;
    return total_nodes;
}

search_t search_result;

// go back through and comment this more to understand it
int qsearch(stack<board_t> *board_stack, int alpha, int beta) {
    vector<move_t> captures;
    board_t *curr_board = &(*board_stack).top();
    hash_val h = curr_board->board_hash;
    /* I don't think I need to do this, since all capture moves could not
       possibly create a position that has already happened */
    // if(game_history.find(h) != game_history.end()) {
    //     return 0;
    // }
    // game_history.insert(h);

    int stand_pat = evaluate(curr_board); // fall back evaluation
    if(stand_pat >= beta) {positions_searched++; return beta;}
    if(alpha < stand_pat) alpha = stand_pat;

    generate_moves(curr_board, &captures, true); // true flag generates only captures
    order_moves(&captures, curr_board, NO_MOVE);
    for (move_t capture : captures) {
        make_move(board_stack, capture);
        int evaluation = -qsearch(board_stack, -beta, -alpha);
        unmake_move(board_stack);

        if(evaluation >= beta) {positions_searched++; return beta;}
        if(evaluation > alpha) alpha = evaluation;
    }
    positions_searched++;
    return alpha;
}

/* DO IT LIKE THAT ONE WEBSITE */
int search(stack<board_t> *board_stack, int ply_from_root, int depth, int alpha, int beta) {
    vector<move_t> moves;
    board_t *curr_board = &(*board_stack).top();
    hash_val h = curr_board->board_hash;

    int flags = ALPHA;

    if(ply_from_root > 0) {
        if(game_history.find(h) != game_history.end()) {
            return 0;
        }
    }

    int original_alpha = alpha;
    /* look up the hash value in the transposition table 
       this will set the tt best move global variable */
    int tt_score = probe_tt_table(h, depth, alpha, beta);
    if(tt_score != FAILED_LOOKUP)
        return tt_score;
    // cout << "here!" << endl;

    if(depth == 0) {
        return qsearch(board_stack, alpha, beta);
        // positions_searched++; 
        // // here I probably want to add boards of depth 0 at some point
        // int static_eval = evaluate(curr_board);
        // return static_eval;
    }
    move_t best_tt_move = TT.best_move;
    generate_moves(curr_board, &moves);
    order_moves(&moves, curr_board, best_tt_move);
    if(moves.size() == 0) {
        if(checking_pieces(curr_board) != 0) {
            return INT_MIN + 1 + ply_from_root; // the deeper in the search we are, the less good the checkmate is
        }
        return 0;
    }

    // game_history.insert(h);
    
    move_t best_move = NO_MOVE;
    for(move_t move : moves) {
        make_move(board_stack, move);
        int evaluation = -search(board_stack, ply_from_root + 1, depth - 1, -beta, -alpha);
        unmake_move(board_stack);
        // I don't think a beta cutoff is possible if ply_from_root == 0
        if(evaluation >= beta) {
            store_entry(h, depth, BETA, beta, move);
            return beta;
        }
        if(evaluation > alpha) {
            flags = EXACT;
            alpha = evaluation;
            best_move = move;
        }
    }

    /* remove this position from the current path for repetition draws */
    // game_history.erase(h);

    /* store this in the transposition table */
    store_entry(h, depth, flags, alpha, best_move);

    if(ply_from_root == 0) {
        search_result.best_move = best_move;
        search_result.score = alpha;
    }
    return alpha;
}

move_t find_best_move(board_t board) {
    /* clear the transposition table */
    clear_tt_table();
    transpositions = 0;
    num_entries = 0;

    hash_val h = board.board_hash;
    game_history.insert(h); // insert the board hash from the user's move

    board_t *next_board;
    stack<board_t> board_stack;
    board_stack.push(board);

    /* check in opening book */
    move_t opening_move = get_opening_move(&board);
    if(opening_move != NO_MOVE) {
        cout << "Played from book!" << endl;

        /* include the move that was made in the history */
        make_move(&board_stack, opening_move);
        game_history.insert(board_stack.top().board_hash);
        unmake_move(&board_stack);
        return opening_move;
    }

    size_t depth = 0;
    int alpha = INT_MIN + 1;
    int beta = INT_MAX;
    clock_t tStart = clock();
    clock_t tStop = clock();
    while((((double)(tStop - tStart)) / CLOCKS_PER_SEC) < 1.0) {
        depth++;
        search(&board_stack, 0, depth, alpha, beta);
        tStop = clock();
        if(search_result.score > 100000) { // mating score
            break;
        }
    }
    cout << "IDSS Depth: " << depth << endl;

    move_t best_move = search_result.best_move;

    int perspective = (board.t == W) ? 1 : -1;
    cout << "Evaluation: " << (search_result.score * perspective) / 100.0 << endl;
    cout << "Transpositions: " << transpositions << endl;
    cout << "Number of entries: " << num_entries << endl;

    /* include the move that was made in the history */
    make_move(&board_stack, best_move);
    game_history.insert(board_stack.top().board_hash);
    unmake_move(&board_stack);

    return best_move;
}

// int main() {
//     luts = init_LUT(); // must do this first
//     zobrist_table = init_zobrist();
//     // opening_book = create_opening_book(); // uncomment if you need to update opening_book
//     // generate_num_data(); // uncomment if you need to update opening_book
//     opening_book = populate_opening_book();
//     string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//     string test_pos_1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//     string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
//     string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
//     string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
//     string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
//     string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

//     board_t board_1 = decode_fen(test_pos_1);
//     board_t board_2 = decode_fen(test_pos_2);
//     board_t board_3 = decode_fen(test_pos_3);
//     board_t board_4 = decode_fen(test_pos_4);
//     board_t board_5 = decode_fen(test_pos_5);
//     board_t board_6 = decode_fen(test_pos_6);

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

//     stack<board_t> board_1_stack;
//     stack<board_t> board_2_stack;
//     stack<board_t> board_3_stack;
//     stack<board_t> board_4_stack;
//     stack<board_t> board_5_stack;
//     stack<board_t> board_6_stack;
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
//             perft(&board_1_stack, depth);
//             cout << endl;

//             cout << "Test 2 at depth " << depth << endl;
//             perft(&board_2_stack, depth);
//             cout << endl;

//             cout << "Test 3 at depth " << depth << endl;
//             perft(&board_3_stack, depth);
//             cout << endl;

//             cout << "Test 4 at depth " << depth << endl;
//             perft(&board_4_stack, depth);
//             cout << endl;

//             cout << "Test 5 at depth " << depth << endl;
//             perft(&board_5_stack, depth);
//             cout << endl;

//             cout << "Test 6 at depth " << depth << endl;
//             perft(&board_6_stack, depth);
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

    maybe the find best move function will just perform the search and then look
    up the hash value in the TT that was created from the search
    still just need to reason through this more

    for speed consider using smaller types for numbers that aren't big

    use an unordered_set and see about replacement strategies
    maybe rename hashing.cpp to transpose.cpp

    use selection sort to order moves after scoring them

    consider code profiling -> wine and valgrind

    woah intel c++ compiler could be huge
    Intel C++ compiler

    check out AMD_code profiler thingy

    not sure if I can get SDL with the intel compiler, also don't know why I 
    wouldn't be able to

    might have to get visual studio in order to get the intel compiler

    no clue how to use visual studio 2019

    next up:
        1. store the results of the openings stuff in the form of the position's hash val and
        the 32 bit number moves
        this way we don't have to generate the moves from the notation every time
        or make the moves
        2. make sure that it doesn't crash
        3. move ordering

    maybe add like a piece least_valuable_attacker(square sq, board_t board)
    which checks pawns, then knights, then bishops, etc...

    ^^^ THIS WOULD BE VERY HELPFUL FOR CHECKING HANGING PIECES 

    CHECK TO SEE IF THE SQUARE THEY ARE COMING FROM IS ATTACKED AND SCORE IT 
    BASED ON THAT AS WELL

    A better evaluation function likely speeds up the program, since you will
    have fewer positions that end up with the same evaluation, meaning there
    should be better alpha beta pruning

    consider adding the position to the set of positions in make_move
    and removing it in unmake_move

    maybe just make it so search returns a pair of eval and move instead of
    the find best move function
*/