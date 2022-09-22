#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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
#include "configuration.h"

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 720

#define BB_PATH "assets/images/bB.svg"
#define BK_PATH "assets/images/bK.svg"
#define BN_PATH "assets/images/bN.svg"
#define BP_PATH "assets/images/bP.svg"
#define BQ_PATH "assets/images/bQ.svg"
#define BR_PATH "assets/images/bR.svg"
#define WB_PATH "assets/images/wB.svg"
#define WK_PATH "assets/images/wK.svg"
#define WN_PATH "assets/images/wN.svg"
#define WP_PATH "assets/images/wP.svg"
#define WQ_PATH "assets/images/wQ.svg"
#define WR_PATH "assets/images/wR.svg"

typedef struct display_square {
    SDL_Rect rect;
    piece pc;
} d_square_t;

typedef struct moving_piece {
    SDL_Rect rect;
    piece pc;
    square start_sq;
} mv_piece_t;

SDL_Window *window;
SDL_Renderer *renderer;

int sq_width = (SCREEN_WIDTH > SCREEN_HEIGHT) ? (SCREEN_HEIGHT / 8) : (SCREEN_WIDTH / 8);

SDL_Texture *Piece_Textures[12];
d_square_t Display_Board[64];




void LoadPieceTextures() {
    Piece_Textures[BLACK_BISHOPS_INDEX] = IMG_LoadTexture(renderer, BB_PATH);
    Piece_Textures[BLACK_KINGS_INDEX] = IMG_LoadTexture(renderer, BK_PATH);
    Piece_Textures[BLACK_KNIGHTS_INDEX] = IMG_LoadTexture(renderer, BN_PATH);
    Piece_Textures[BLACK_PAWNS_INDEX] = IMG_LoadTexture(renderer, BP_PATH);
    Piece_Textures[BLACK_QUEENS_INDEX] = IMG_LoadTexture(renderer, BQ_PATH);
    Piece_Textures[BLACK_ROOKS_INDEX] = IMG_LoadTexture(renderer, BR_PATH);

    Piece_Textures[WHITE_BISHOPS_INDEX] = IMG_LoadTexture(renderer, WB_PATH);
    Piece_Textures[WHITE_KINGS_INDEX] = IMG_LoadTexture(renderer, WK_PATH);
    Piece_Textures[WHITE_KNIGHTS_INDEX] = IMG_LoadTexture(renderer, WN_PATH);
    Piece_Textures[WHITE_PAWNS_INDEX] = IMG_LoadTexture(renderer, WP_PATH);
    Piece_Textures[WHITE_QUEENS_INDEX] = IMG_LoadTexture(renderer, WQ_PATH);
    Piece_Textures[WHITE_ROOKS_INDEX] = IMG_LoadTexture(renderer, WR_PATH);
}

void DrawChessBoard() {
    SDL_Rect sq;
    sq.w = sq_width;
    sq.h = sq_width;
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            if((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
            }
            sq.y = row * sq_width;
            sq.x = col * sq_width;
            SDL_RenderFillRect(renderer, &sq);
        }
    }
}

SDL_Rect GetRect(int row, int col) {
    SDL_Rect rect; 
    rect.x = sq_width * col;
    rect.y = sq_width * row;
    rect.h = sq_width;
    rect.w = sq_width;
    return rect;
}

void LoadDisplayBoardFromGameState(piece sq_board[64]) {  
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            square sq = (square)((7-row) * 8 + col);
            piece pc = sq_board[sq];
            SDL_Rect rect = GetRect(row, col);
            Display_Board[sq].rect = rect;
            if(pc != EMPTY) {
                Display_Board[sq].pc = pc;
            }
            else {
                Display_Board[sq].pc = EMPTY;
            }
        }
    }
}

void DrawPieces() {
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            square sq = (square)((7-row) * 8 + col);
            d_square_t display_square = Display_Board[sq];
            if(display_square.pc != EMPTY) {
                SDL_RenderCopy(renderer, Piece_Textures[INDEX_FROM_PIECE(display_square.pc)], NULL, &(display_square.rect));
            }
        }
    }
    
}

void DrawSelectedPiece(mv_piece_t selectedPiece) {
    if(selectedPiece.pc != EMPTY) {
        SDL_RenderCopy(renderer, Piece_Textures[INDEX_FROM_PIECE(selectedPiece.pc)], NULL, &(selectedPiece.rect));
    }
}

d_square_t *SquareFromLoc(SDL_Point mousePos) {
    int row = mousePos.y / sq_width;
    int col = mousePos.x / sq_width;
    // add bounds checking here
    return &Display_Board[(7-row) * 8 + col];
}

square SquareNumFromLoc(SDL_Point mousePos) {
    return (square)((7 - (mousePos.y / sq_width)) * 8 + (mousePos.x / sq_width));
}


int test_main() {
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

int main(int argc, char** argv){
    luts = init_LUT(); // must do this first
    zobrist_table = init_zobrist();
    // opening_book = create_opening_book(); // uncomment if you need to update opening_book
    // generate_num_data(); // uncomment if you need to update opening_book
    opening_book = populate_opening_book();
    init_tt_table();
    init_eval_table();

    load_settings_from_config();

    if(TEST_MODE) {
        test_main();
        return 0;
    }

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("Error: SDL failed to initialize\nSDL Error: '%s'\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Cbot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window){
        printf("Error: Failed to open window\nSDL Error: '%s'\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer){
        printf("Error: Failed to create renderer\nSDL Error: '%s'\n", SDL_GetError());
        return 1;
    }

    int flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if(IMG_Init(flags) != flags) {
        printf("Error: Failed to create renderer\nSDL Error: '%s'\n", SDL_GetError());
        return 1;
    }

    LoadPieceTextures();

    decode_fen(FEN_STRING);
    
    move_t move;
    vector<move_t> legal_moves;
    generate_moves(legal_moves);
 
    LoadDisplayBoardFromGameState(b.sq_board);
    
    mv_piece_t selectedPiece;
    selectedPiece.pc = EMPTY;

    bool leftMouseButtonDown = false;
    bool madeMove = false;
    SDL_Point mousePos;
    bool running = true;
    square to;
    square from;
    piece mv_piece;
    piece tar_piece;
    piece promotion_piece;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_MOUSEMOTION:
                    mousePos = {event.motion.x, event.motion.y};
                    if(leftMouseButtonDown && selectedPiece.pc != EMPTY) {
                        selectedPiece.rect.x = mousePos.x - (sq_width / 2);
                        selectedPiece.rect.y = mousePos.y - (sq_width / 2);
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if(leftMouseButtonDown && event.button.button == SDL_BUTTON_LEFT) {
                        if(selectedPiece.pc != EMPTY) {
                            d_square_t *d_sq = SquareFromLoc(mousePos);
                            to = SquareNumFromLoc(mousePos);
                            tar_piece = d_sq->pc;
                            int flags;

                            if(PIECE(mv_piece) == PAWN && (RANK(to) == RANK_8 || RANK(to) == RANK_1)) {
                                if(tar_piece != EMPTY) {
                                    flags = QUEEN_PROMO_CAPTURE;
                                }
                                else {
                                    flags = QUEEN_PROMO;
                                }
                            }
                            else if(PIECE(mv_piece) == PAWN && (abs(RANK(from) - RANK(to)) == 2)) {
                                flags = DOUBLE_PUSH;
                            }
                            else if(PIECE(mv_piece) == PAWN && to == EN_PASSANT_SQ(b.state_history.top())) {
                                flags = EN_PASSANT_CAPTURE;
                            }
                            else if(PIECE(mv_piece) == KING && (FILE(to) - FILE(from) == 2)) {
                                flags = KING_SIDE_CASTLE;
                            }
                            else if(PIECE(mv_piece) == KING && (FILE(to) - FILE(from) == -2)) {
                                flags = QUEEN_SIDE_CASTLE;
                            }
                            else if(tar_piece != EMPTY) {
                                flags = NORMAL_CAPTURE;
                            }
                            else {
                                flags = QUIET_MOVE;
                            }
                            move = construct_move(from, to, flags);
                            for (move_t legal_move : legal_moves) {
                                if(legal_move == move) {
                                    make_move(move);
                                    madeMove = true;
                                    break;
                                }
                            }
                            if(!madeMove){
                                Display_Board[selectedPiece.start_sq].pc = selectedPiece.pc; // put the piece back where you picked it up
                            }
                
                        }
                        leftMouseButtonDown = false;
                        selectedPiece.pc = EMPTY;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (!leftMouseButtonDown && event.button.button == SDL_BUTTON_LEFT) {
                        leftMouseButtonDown = true;
                        d_square_t *d_sq = SquareFromLoc(mousePos);
                        from = SquareNumFromLoc(mousePos);
                        
                        if(d_sq->pc != EMPTY) {
                            selectedPiece.pc = d_sq->pc; // set whats inside equal to each other
                            selectedPiece.start_sq = from;
                            selectedPiece.rect = d_sq->rect;
                            d_sq->pc = EMPTY;
                            mv_piece = selectedPiece.pc;
                        }
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT) {
                        if(b.state_history.size() > 1) {
                            // game_history.erase(b.board_hash);
                            unmake_move(LAST_MOVE(b.state_history.top()));
                            LoadDisplayBoardFromGameState(b.sq_board);
                            legal_moves.clear();
                            generate_moves(legal_moves);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        // cout << b.state_history.size() << endl;

        if(madeMove) {
            LoadDisplayBoardFromGameState(b.sq_board);
            SDL_RenderClear(renderer);
            DrawChessBoard();
            DrawPieces();
            DrawSelectedPiece(selectedPiece);
            SDL_RenderPresent(renderer);

            legal_moves.clear();
            generate_moves(legal_moves);
            if(legal_moves.size() == 0) {
                if(in_check()) {
                    cout << "Checkmate!" << endl;
                    break;
                }
                cout << "Stalemate!" << endl;
                break;
            }

            move = find_best_move();
            make_move(move);
            LoadDisplayBoardFromGameState(b.sq_board);

            legal_moves.clear();
            generate_moves(legal_moves);

            madeMove = false;
        }

        /* code to play itself */
        if(AIVSAI) {
            LoadDisplayBoardFromGameState(b.sq_board);
            SDL_RenderClear(renderer);
            DrawChessBoard();
            DrawPieces();
            DrawSelectedPiece(selectedPiece);
            SDL_RenderPresent(renderer);

            legal_moves.clear();
            generate_moves(legal_moves);
            if(legal_moves.size() == 0) {
                if(checking_pieces()) {
                    cout << "Checkmate!" << endl;
                    break;
                }
                cout << "Stalemate!" << endl;
                break;
            }

            move = find_best_move();
            make_move(move); // leaking memory here
            LoadDisplayBoardFromGameState(b.sq_board);

            legal_moves.clear();
            generate_moves(legal_moves);
        }
        /* code to play itself */

        SDL_RenderClear(renderer);
        DrawChessBoard();
        DrawPieces();
        DrawSelectedPiece(selectedPiece);
        SDL_RenderPresent(renderer);

        if(legal_moves.size() == 0) {
            if(in_check()) {
                cout << "Checkmate!" << endl;
                break;
            }
            cout << "Stalemate!" << endl;
            break;
        }
    }

    int test;
    cin >> test; // wait to close

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    free_tt_table();
    free_eval_table();
    return 0;
}