#include "include/search.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/moves.h"
#include "include/evaluation.h"
#include "include/hashing.h"
#include "include/openings.h"
#include "include/debugging.h"
#include "include/attacks.h"
#include "include/tt.h"

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
    generate_moves(moves);
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
    generate_moves(moves);
    for(move_t move : moves) {
        make_move(move);
        total_moves += num_nodes(depth - 1); 
        unmake_move(move);
    }
    return total_moves;
}

uint64_t perft(size_t depth) {
    vector<move_t> moves;
    generate_moves(moves);
    uint64_t total_nodes = 0;
    uint64_t nodes_from_move = 0;
    sort_by_algebraic_notation(moves);
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
    int stand_pat = evaluate(alpha, beta); // fall back evaluation
    if(stand_pat >= beta) {nodes_reached++; return beta;}
    if(alpha < stand_pat) alpha = stand_pat;

    generate_moves(captures, true); // true flag generates only captures
    order_moves(captures, NO_MOVE); /* I could make an order capture functions that I call here to not waste time */
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

    if(ply_from_root > 0 && is_repetition()) {
        return 0;
    }

    /* 
        Null Move Pruning:
            - Runs on the assumption that if you give your opponent a free move, and the resulting
            shorter-depth search still causes a cutoff, it is very likely the initial position would have 
            caused a cutoff.
            - Because of ZugZwang in the endgame, where making a null move can be good, this pruning
            is turned off in the endgame.
    */
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
    generate_moves(moves);
    order_moves(moves, NO_SCORE(best_tt_move));
    
    move_t best_move = NO_MOVE;
    int num_moves = moves.size();
    move_t move;
    int evaluation;
    bool pv_search = true;
    int promotion_extension = 0;
    for(int i = 0; i < num_moves; i++) {
        move = moves[i];
        /* searches lines deeper where you push a pawn close to promotion (or promote it)! */
        promotion_extension = 0;
        if(pawn_promo_or_close_push(move)) promotion_extension = 1;
        make_move(move);
        /*
            Principal Variation Search (PVS):
                - There is only one pathway of moves that are acceptable for both players in any given search.
                - All we have to do in non PV nodes is prove that they are not acceptable for either player.
                - If we are wrong about being in a PV node, a costly re-search is required.
        */

        if(pv_search) {
            evaluation = -search_moves(ply_from_root + 1, depth - 1 + promotion_extension, -beta, -alpha, IS_PV, CAN_NULL);
        }
        else {
            evaluation = -search_moves(ply_from_root + 1, depth - 1 + promotion_extension, -alpha - 1, -alpha, NO_PV, CAN_NULL);
            if(evaluation > alpha) {
                evaluation = -search_moves(ply_from_root + 1, depth - 1 + promotion_extension, -beta, -alpha, IS_PV, CAN_NULL);
            }
        }
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
    }

    nodes_reached++;

    /* store this in the transposition table */
    store_entry(h, depth, ply_from_root, flags, alpha, best_move);

    if(ply_from_root == 0)
        search_complete = true;
    return alpha;
}

move_t find_best_move(int search_time) {
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

    /* check in opening book */
    move_t opening_move = get_opening_move();
    if(opening_move != NO_MOVE) {
        this_thread::sleep_for(chrono::milliseconds(search_time));
        std::cout << "Depth: None | Evaluation: Book | Move: ";
        std::cout << notation_from_move(opening_move) << endl;
        return opening_move;
    }

    size_t depth = 0;
    int alpha = INT_MIN + 1;
    int beta = INT_MAX;
    clock_t tStart = clock();
    clock_t tStop = clock();
    abort_search = false;
    search_complete = true;
    search_result.score = 0;
    search_result.best_move = NO_MOVE;
    thread t;

    /* iterative deepening framework with threading */
    while(true) {
        tStop = clock();
        if((((double)(tStop - tStart)) / CLOCKS_PER_SEC) > ((double)search_time / 1000)){
            abort_search = true;
            break;
        }
        if(search_complete) {
            if(t.joinable())
                t.join();
            search_complete = false;
            depth++;
            t = std::thread{search_moves, 0, depth, alpha, beta, CAN_NULL, IS_PV};
        }
    }
    /* wait for any lingering thread to finish */
    if(t.joinable())
        t.join();

    float time_elapsed = (tStop - tStart);
    std::cout << "Depth: " << depth << " | ";

    move_t best_move = search_result.best_move;
    int perspective = (b.t == W) ? 1 : -1;
    int score = search_result.score * perspective;
    if(is_mate_score(score) && score > 0)
        std::cout << "Evaluation: White has mate in " << moves_until_mate(score) << " move(s) | ";
    else if(is_mate_score(score) && score < 0)
        std::cout << "Evaluation: Black has mate in " << moves_until_mate(score) << " move(s) | ";
    else
        std::cout << "Evaluation: " << score / 100.0 << " | ";
    std::cout << "Move: " << notation_from_move(best_move) << endl;
    return best_move;
}