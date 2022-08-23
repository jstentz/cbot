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
#include <thread>

using namespace std;

size_t nodes_reached = 0; // speed test purposes
size_t beta_cutoffs = 0;
size_t q_nodes = 0;

bool search_complete = false;
bool abort_search = false;

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
        total_moves += num_nodes_bulk(depth - 1); 
        unmake_move(move);
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
    uint64_t total_nodes = 0;
    uint64_t nodes_from_move = 0;
    sort_by_algebraic_notation(&moves);
    for(move_t move : moves) {
        std::cout << algebraic_notation(move) << ": ";
        make_move(move);
        nodes_from_move = num_nodes_bulk(depth - 1);
        total_nodes += nodes_from_move;
        std::cout << nodes_from_move << endl;
        unmake_move(move);
    }
    std::cout << "Nodes searched: " << total_nodes << endl;
    return total_nodes;
}

search_t search_result;

int qsearch(int alpha, int beta) {
    vector<move_t> captures;
    hash_val h = b.board_hash;

    /**
     * Since none of these captures are forced, meaning a player doesn't
     * have to make that capture, we can use this evaluation to represent
     * them not taking the piece. We will see if it is better or worse for
     * them to make that capture.
     */
    int stand_pat = evaluate(); // fall back evaluation
    if(stand_pat >= beta) {nodes_reached++; return beta;}
    if(alpha < stand_pat) alpha = stand_pat;

    generate_moves(&captures, true); // true flag generates only captures
    order_moves(&captures, NO_MOVE); /* I could make an order capture functions that I call here to not waste time */
    for (move_t capture : captures) {
        /* delta pruning helps to stop searching helpless nodes */
        // piece captured_piece = b.sq_board[TO(capture)];
        if(is_bad_capture(capture)) /* don't consider captures that are bad */
            continue;
        make_move(capture);
        int evaluation = -qsearch(-beta, -alpha);
        unmake_move(capture);

        if(evaluation >= beta) {nodes_reached++; beta_cutoffs++; return beta;}
        if(evaluation > alpha) alpha = evaluation;
    }
    nodes_reached++;
    return alpha;
}

int search_moves(int ply_from_root, int depth, int alpha, int beta, bool is_pv, bool can_null) {
    if(abort_search)
        return 0;

    vector<move_t> moves;
    hash_val h = b.board_hash;

    int flags = ALPHA;

    if(ply_from_root > 0) {
        /* check for repetition draw */
        if(probe_game_history(h))
            return 0;
    }

    bool check_flag;
    if(!can_null) /* if we just made a null move (passed the turn), we cannot be in check */
        check_flag = false;
    else
        check_flag = in_check();

    /* check extension */
    if(check_flag)
        depth++;

    /* look up the hash value in the transposition table 
       this will set the tt best move global variable */
    int tt_score = probe_tt_table(h, depth, ply_from_root, alpha, beta);
    if(tt_score != FAILED_LOOKUP) {
        nodes_reached++;
        return tt_score;
    }

    if(depth == 0) {
        return qsearch(alpha, beta);
    }

    if(depth > 2 && 
       can_null && 
       !is_pv && 
       b.total_material > ENDGAME_MATERIAL &&
       !check_flag) {
        int reduce = 2;
        if(depth > 6) reduce = 3;
        make_nullmove();
        int score = -search_moves(ply_from_root, depth - reduce - 1, -beta, -beta + 1, NO_PV, NO_NULL);
        unmake_nullmove();
        if(abort_search) return 0;
        store_entry(h, depth, ply_from_root, BETA, beta, NO_MOVE);
        if(score >= beta) return beta;
    }

    move_t best_tt_move = TT.best_move;
    generate_moves(&moves);
    order_moves(&moves, NO_SCORE(best_tt_move));
    
    move_t best_move = NO_MOVE;
    int num_moves = moves.size();
    move_t move;
    int evaluation;
    bool pv_search = true;
    for(int i = 0; i < num_moves; i++) {
        move = moves[i];
        make_move(move);
        if(pv_search) {
            evaluation = -search_moves(ply_from_root + 1, depth - 1, -beta, -alpha, IS_PV, CAN_NULL);
        }
        else {
            evaluation = -search_moves(ply_from_root + 1, depth - 1, -alpha - 1, -alpha, NO_PV, CAN_NULL);
            if(evaluation > alpha) {
                evaluation = -search_moves(ply_from_root + 1, depth - 1, -beta, -alpha, IS_PV, CAN_NULL);
            }
        }

        // evaluation = -search_moves(ply_from_root + 1, depth - 1, -beta, -alpha);
            
        unmake_move(move);

        if(abort_search)
            return 0;
        
        if(evaluation >= beta) {
            store_entry(h, depth, ply_from_root, BETA, beta, move);
            nodes_reached++;
            beta_cutoffs++;
            return beta;
        }
        /* found a new best move here! */
        if(evaluation > alpha) {
            flags = EXACT;
            alpha = evaluation;
            best_move = move;
            /* if we are at the root node, replace the best move we've seen so far */
            if(ply_from_root == 0 && !abort_search) {
                search_result.best_move = best_move;
                search_result.score = alpha;
            }
        }
        pv_search = false;
    }

    if(num_moves == 0) {
        nodes_reached++;
        if(check_flag) { /* checkmate */
            checkmates++;
            alpha = INT_MIN + 1 + ply_from_root; // the deeper in the search we are, the less good the checkmate is
        }
        else {
            /* stalemate! */
            alpha = 0;
        }
        flags = EXACT; /* we know the exact score of checkmated or stalemated positions */
        depth = INT_MAX; /* this position is searched to the best depth if we are in checkmate or stalemate */
    }

    nodes_reached++;

    /* store this in the transposition table */
    store_entry(h, depth, ply_from_root, flags, alpha, best_move);

    if(ply_from_root == 0)
        search_complete = true;
    return alpha;
}

int simple_search(int ply_from_root, int depth, int alpha, int beta) {
    if(abort_search)
        return 0;

    vector<move_t> moves;
    hash_val h = b.board_hash;

    int flags = ALPHA;

    // if(ply_from_root > 0) {
    //     /* check for repetition draw */
    //     if(probe_game_history(h))
    //         return 0;
    // }

    bool check_flag;
    check_flag = in_check();

    /* look up the hash value in the transposition table 
       this will set the tt best move global variable */
    int tt_score = probe_tt_table(h, depth, ply_from_root, alpha, beta);
    if(tt_score != FAILED_LOOKUP) {
        nodes_reached++;
        return tt_score;
    }

    if(depth == 0) {
        return evaluate();
    }

    move_t best_tt_move = TT.best_move;
    generate_moves(&moves);
    order_moves(&moves, NO_SCORE(best_tt_move));
    
    move_t best_move = NO_MOVE;
    int num_moves = moves.size();
    move_t move;
    int evaluation;
    for(int i = 0; i < num_moves; i++) {
        move = moves[i];
        make_move(move);
        evaluation = -simple_search(ply_from_root + 1, depth - 1, -beta, -alpha);
        unmake_move(move);

        if(abort_search)
            return 0;
        
        if(evaluation >= beta) {
            store_entry(h, depth, ply_from_root, BETA, beta, move);
            nodes_reached++;
            beta_cutoffs++;
            return beta;
        }
        /* found a new best move here! */
        if(evaluation > alpha) {
            flags = EXACT;
            alpha = evaluation;
            best_move = move;
            /* if we are at the root node, replace the best move we've seen so far */
            if(ply_from_root == 0 && !abort_search) {
                search_result.best_move = best_move;
                search_result.score = alpha;
            }
        }
    }

    if(num_moves == 0) {
        nodes_reached++;
        if(check_flag) { /* checkmate */
            checkmates++;
            // alpha = INT_MIN + 1 + ply_from_root; // the deeper in the search we are, the less good the checkmate is
            return INT_MIN + 1 + ply_from_root; 
        }
        else {
            /* stalemate! */
            // alpha = 0;
            return 0;
        }
        // flags = EXACT; /* we know the exact score of checkmated or stalemated positions */
        // depth = INT_MAX; /* this position is searched to the best depth if we are in checkmate or stalemate */
    }

    nodes_reached++;

    /* store this in the transposition table */
    store_entry(h, depth, ply_from_root, flags, alpha, best_move);

    if(ply_from_root == 0)
        search_complete = true;
    return alpha;
}

/*
IMPORTANT NOTE:

I MUST CORRECT THE MATE SCORES IN THE TT BECAUSE THE PLY FROM ROOT IS DIFFERENT 
DEPENDING ON THE PATH WE GOT TO THAT POSITION, SO THE MATING SCORE WOULD BE DIFFERENT

CHECK SEBLAGUES CODE

https://github.com/SebLague/Chess-AI/blob/main/Assets/Scripts/Core/TranspositionTable.cs
https://github.com/SebLague/Chess-AI/blob/main/Assets/Scripts/Core/AI/Search.cs


*/

move_t find_best_move() {
    /* clear the eval table */
    clear_eval_table();
    eval_hits = 0;
    eval_probes = 0;
    /* clear the transposition table */
    clear_tt_table();
    tt_hits = 0;
    tt_probes = 0;

    nodes_reached = 0;
    beta_cutoffs = 0;

    hash_val h = b.board_hash;
    game_history.insert(h); // insert the board hash from the user's move

    int search_time = 1250;

    /* check in opening book */
    move_t opening_move = get_opening_move();
    if(opening_move != NO_MOVE) {
        this_thread::sleep_for(chrono::milliseconds(search_time));
        std::cout << "Played from book!" << endl << endl;

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
    // (((double)(tStop - tStart)) / CLOCKS_PER_SEC) < 1.0
    abort_search = false;
    search_complete = true;
    search_result.score = 0;
    search_result.best_move = NO_MOVE;
    thread t;
    while(true) {
        // cout << search_complete << endl;
        tStop = clock();
        if((((double)(tStop - tStart)) / CLOCKS_PER_SEC) > ((double)search_time / 1000)){
            abort_search = true;
            // cout << "aborted!" << endl;
            break;
        }
        if(search_complete) {
            // cout << "search completed!" << endl;
            if(t.joinable())
                t.join();
            search_complete = false;
            depth++;
            t = std::thread{search_moves, 0, depth, alpha, beta, CAN_NULL, IS_PV};
            // t = std::thread{simple_search, 0, depth, alpha, beta};
            // cout << notation_from_move(search_result.best_move) << endl;
        }
        
        // if(is_mate_score(search_result.score)) { // mating score
        //     abort_search = true;
        //     break;
        // }
    }
    if(t.joinable())
        t.join();

    // cout << "After waiting for thread to close: " << notation_from_move(search_result.best_move) << endl;
    // t.join(); /* once we abort wait for it to finish */
    float time_elapsed = (tStop - tStart);
    std::cout << "IDDFS Depth: " << depth << endl;

    move_t best_move = search_result.best_move;

    int perspective = (b.t == W) ? 1 : -1;
    if(is_mate_score(search_result.score))
        std::cout << "Evaluation: Mate in " << moves_until_mate(search_result.score) << " move(s)" << endl;
    else
        std::cout << "Evaluation: " << (search_result.score * perspective) / 100.0 << endl;
    std::cout << "Transposition hit percentage: " << ((float)tt_hits / (float)tt_probes * 100.0) << endl;
    std::cout << "Eval hit percentage: " << ((float)eval_hits / (float)eval_probes * 100.0) << endl;
    std::cout << "Leaf-nodes reached: " << nodes_reached << endl;
    std::cout << "Beta cutoffs: " << beta_cutoffs << endl << endl;

    /* include the move that was made in the history */
    /* MOVE THIS SOMEWHERE OUTSIDE OF THIS FUNCTION */
    make_move(best_move);
    game_history.insert(b.board_hash);
    unmake_move(best_move);

    return best_move;
}

// int main() {
//     luts = init_LUT(); // must do this first
//     zobrist_table = init_zobrist();
//     // opening_book = create_opening_book(); // uncomment if you need to update opening_book
//     // generate_num_data(); // uncomment if you need to update opening_book
//     opening_book = populate_opening_book();
//     init_tt_table();
//     init_eval_table();
//     string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//     string test_pos_1 = "8/6k1/8/5RK1/8/8/8/8 b - - 0 1";
//     string test_pos_2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
//     string test_pos_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
//     string test_pos_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
//     string test_pos_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
//     string test_pos_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

//     size_t depth;
//     size_t total_nodes;
//     clock_t tStart;
//     clock_t tStop;
//     double time_elapsed;

//     char answer;
//     string fen;
//     while(true) {
//         cout << "Perft test or speed test or move test? (p/s/m)" << endl;
//         cin >> answer;
//         if(answer == 'p') {
//             cout << endl << "Enter depth: ";
//             cin >> depth;

//             decode_fen(test_pos_1);
//             cout << "Test 1 at depth " << depth << endl;
//             perft(depth);
//             cout << endl;

//             decode_fen(test_pos_2);
//             cout << "Test 2 at depth " << depth << endl;
//             perft(depth);
//             cout << endl;

//             decode_fen(test_pos_3);
//             cout << "Test 3 at depth " << depth << endl;
//             perft(depth);
//             cout << endl;

//             decode_fen(test_pos_4);
//             cout << "Test 4 at depth " << depth << endl;
//             perft(depth);
//             cout << endl;

//             decode_fen(test_pos_5);
//             cout << "Test 5 at depth " << depth << endl;
//             perft(depth);
//             cout << endl;

//             decode_fen(test_pos_6);
//             cout << "Test 6 at depth " << depth << endl;
//             perft(depth);
//             cout << endl;
//         }
//         else if(answer == 's') {
//             cout << endl << "Enter depth: ";
//             cin >> depth;
//             total_nodes = 0;
//             tStart = clock();
//             /* having to decode the fen in between will slow it down */
//             /* rework this for that reason */
//             decode_fen(test_pos_1);
//             total_nodes += num_nodes_bulk(depth);
//             decode_fen(test_pos_2);
//             total_nodes += num_nodes_bulk(depth);
//             decode_fen(test_pos_3);
//             total_nodes += num_nodes_bulk(depth);
//             decode_fen(test_pos_4);
//             total_nodes += num_nodes_bulk(depth);
//             decode_fen(test_pos_5);
//             total_nodes += num_nodes_bulk(depth);
//             decode_fen(test_pos_6);
//             total_nodes += num_nodes_bulk(depth);
//             tStop = clock();
//             time_elapsed = (double)(tStop - tStart)/CLOCKS_PER_SEC;
//             cout << "Total nodes: " << total_nodes << endl;
//             cout << "Time elapsed: " << time_elapsed << endl;
//             cout << "Nodes per second: " << ((double)total_nodes / time_elapsed) << endl << endl;
//         }
//         else if(answer == 'm') {
//             fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
//             decode_fen(fen);
//             cout << endl;
//             print_squarewise(b.sq_board);
//             cout << endl;
//             move_t move = find_best_move();
//             cout << endl << "Best move: ";
//             cout << notation_from_move(move) << endl;
//         }
//     }
//     free_tt_table();
//     free_eval_table();
//     return 0;
// }