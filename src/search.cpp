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

uint64_t num_nodes_bulk(size_t depth) {
    vector<move_t> moves;
    generate_moves(&moves);
    if(depth == 1) {
        return moves.size();
    }
    else if(depth == 0) {
        return 1;
    }

    uint64_t total_moves = 0;
    for(move_t move : moves) {
        make_move(move);
        // cout << "Move made: " << endl;
        // print_squarewise(b.sq_board);
        // cout << endl;
        total_moves += num_nodes_bulk(depth - 1); 
        unmake_move(move);
        // cout << "Move unmade: " << endl;
        // print_squarewise(b.sq_board);
        // cout << endl;
    }
    return total_moves;
}

uint64_t num_nodes(size_t depth) {
    if(depth == 0) {
        return 1;
    }

    uint64_t total_moves = 0;
    vector<move_t> moves;
    generate_moves(&moves);
    for(move_t move : moves) {
        make_move(move);
        total_moves += num_nodes(depth - 1); 
        unmake_move(move);
    }
    return total_moves;
}

uint64_t perft(size_t depth) {
    vector<move_t> moves;
    generate_moves(&moves);
    uint64_t total_nodes = 0; // this can overflow, should change
    uint64_t nodes_from_move = 0;
    for(move_t move : moves) {
        cout << notation_from_move(move) << ": ";
        // cout << hex << move << dec << endl;
        make_move(move);
        // cout << "Move made: " << endl;
        // print_squarewise(b.sq_board);
        // cout << endl;
        nodes_from_move = num_nodes_bulk(depth - 1);
        total_nodes += nodes_from_move;
        cout << nodes_from_move << endl;
        unmake_move(move);
        // cout << "Move unmade: " << endl;
        // print_squarewise(b.sq_board);
        // cout << endl;
    }
    cout << "Nodes searched: " << total_nodes << endl;
    return total_nodes;
}

search_t search_result;

// never really checked this for bugs
int qsearch(int alpha, int beta) {
    vector<move_t> captures;
    hash_val h = b.board_hash;

    int stand_pat = evaluate(); // fall back evaluation
    if(stand_pat >= beta) {positions_searched++; return beta;}
    if(alpha < stand_pat) alpha = stand_pat;

    generate_moves(&captures, true); // true flag generates only captures
    order_moves(&captures, NO_MOVE);
    for (move_t capture : captures) {
        // cout << notation_from_move(capture, captures, curr_board) << endl;
        make_move(capture);
        int evaluation = -qsearch(-beta, -alpha);
        unmake_move(capture);

        if(evaluation >= beta) {positions_searched++; return beta;}
        if(evaluation > alpha) alpha = evaluation;
    }
    positions_searched++;
    return alpha;
}

/* DO IT LIKE THAT ONE WEBSITE */
int search(int ply_from_root, int depth, int alpha, int beta) {
    vector<move_t> moves;
    hash_val h = b.board_hash;

    int flags = ALPHA;

    if(ply_from_root > 0) {
        if(probe_game_history(h)) {
            // print_squarewise(curr_board->sq_board);
            // cout << endl;
            return 0;
        }
    }

    /* look up the hash value in the transposition table 
       this will set the tt best move global variable */
    int tt_score = probe_tt_table(h, depth, alpha, beta);
    if(tt_score != FAILED_LOOKUP)
        return tt_score;
    // cout << "here!" << endl;

    if(depth == 0) {
        return qsearch(alpha, beta);
        // positions_searched++; 
        // // here I probably want to add boards of depth 0 at some point
        // int static_eval = evaluate(curr_board);
        // return static_eval;
    }
    move_t best_tt_move = TT.best_move;
    generate_moves(&moves);
    order_moves(&moves, best_tt_move);
    if(moves.size() == 0) {
        if(checking_pieces() != 0) {
            checkmates++;
            return INT_MIN + 1 + ply_from_root; // the deeper in the search we are, the less good the checkmate is
        }
        // cout << "stalemate!" << endl;
        return 0;
    }

    // game_history.insert(h);
    
    move_t best_move = NO_MOVE;
    for(move_t move : moves) {
        make_move(move);
        int evaluation = -search(ply_from_root + 1, depth - 1, -beta, -alpha);
        unmake_move(move);
        
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

move_t find_best_move() {
    /* clear the eval table */
    clear_eval_table();
    eval_hits = 0;
    eval_probes = 0;
    /* clear the transposition table */
    clear_tt_table();
    tt_hits = 0;
    tt_probes = 0;

    hash_val h = b.board_hash;
    game_history.insert(h); // insert the board hash from the user's move

    /* check in opening book */
    move_t opening_move = get_opening_move();
    if(opening_move != NO_MOVE) {
        cout << "Played from book!" << endl << endl;

        /* include the move that was made in the history */
        make_move(opening_move);
        game_history.insert(b.board_hash);
        unmake_move(opening_move);
        return opening_move;
    }

    size_t depth = 0;
    int alpha = INT_MIN + 1;
    int beta = INT_MAX;
    clock_t tStart = clock();
    clock_t tStop = clock();
    while((((double)(tStop - tStart)) / CLOCKS_PER_SEC) < 1.0) {
        checkmates = 0;
        depth++;
        search(0, depth, alpha, beta);
        tStop = clock();
        if(search_result.score > 100000) { // mating score
            break;
        }
    }
    float time_elapsed = (tStop - tStart);
    // search(&board_stack, 0, 15, alpha, beta);
    cout << "IDDFS Depth: " << depth << endl;

    move_t best_move = search_result.best_move;

    int perspective = (b.t == W) ? 1 : -1;
    cout << "Evaluation: " << (search_result.score * perspective) / 100.0 << endl;
    cout << "Transposition hit percentage: " << ((float)tt_hits / (float)tt_probes * 100.0) << endl;
    cout << "Eval hit percentage: " << ((float)eval_hits / (float)eval_probes * 100.0) << endl;
    cout << "Time elapsed: " << time_elapsed << endl << endl;

    /* include the move that was made in the history */
    /* MOVE THIS SOMEWHERE OUTSIDE OF THIS FUNCTION */
    make_move(best_move);
    game_history.insert(b.board_hash);
    unmake_move(best_move);

    return best_move;
}

int main() {
    luts = init_LUT(); // must do this first
    zobrist_table = init_zobrist();
    // opening_book = create_opening_book(); // uncomment if you need to update opening_book
    // generate_num_data(); // uncomment if you need to update opening_book
    // opening_book = populate_opening_book();
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
    while(true) {
        cout << "Perft test or speed test or move test? (p/s/m)" << endl;
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
            fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
            decode_fen(fen);
            cout << endl;
            print_squarewise(b.sq_board);
            cout << endl;
            move_t move = find_best_move();
            cout << endl << "Best move: ";
            cout << notation_from_move(move) << endl;
        }
    }
    free_tt_table();
    free_eval_table();
    return 0;
}

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

    need to have a penalty for pinned pieces in terms of mobility maybe
    calculate middlegame mobility scores weighted higher for bishops and knights
    calculate endgame mobility scores weighted higher for rooks and queens

    threading so I can kill the thread

    revisit move ordering and make sure its fixed with new game phase

    look at how the CPW-Engine does the sizing for the transposition table

    test 4 is wrong... probably a promotion problem

    very very strange bug with promotion captures
*/