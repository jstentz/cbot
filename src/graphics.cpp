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

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 720

#define BB_PATH "assets/bB.svg"
#define BK_PATH "assets/bK.svg"
#define BN_PATH "assets/bN.svg"
#define BP_PATH "assets/bP.svg"
#define BQ_PATH "assets/bQ.svg"
#define BR_PATH "assets/bR.svg"
#define WB_PATH "assets/wB.svg"
#define WK_PATH "assets/wK.svg"
#define WN_PATH "assets/wN.svg"
#define WP_PATH "assets/wP.svg"
#define WQ_PATH "assets/wQ.svg"
#define WR_PATH "assets/wR.svg"

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
                SDL_SetRenderDrawColor(renderer, 238, 238, 210, 0xFF);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 118, 150, 86, 0xFF);
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

int main(int argc, char** argv){
    luts = init_LUT(); // must do this first
    zobrist_table = init_zobrist();
    // opening_book = create_opening_book(); // uncomment if you need to update opening_book
    // generate_num_data(); // uncomment if you need to update opening_book
    opening_book = populate_opening_book();
    init_tt_table();
    init_eval_table();
    


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

    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    // SDL_SetHint(SDL_HINT_RENDER_BATCHING, "2");

    LoadPieceTextures();

    // decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // decode_fen("k7/8/3p4/p2P1p2/P2P1P2/8/8/K7 b - - 0 1"); /* drawn KP endgame */
    // decode_fen("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 1 2"); /* KP endgame winning for white */
    // decode_fen("8/k7/3p4/p2P1p2/P2P1P2/8/1K6/8 b - - 2 2"); /* KP endgame drawn */
    // decode_fen("k7/8/8/8/8/8/8/4BNK w - - 1 2"); /* bishop and knight checkmate */
    // decode_fen("8/4k3/8/8/8/5P2/5K2/8 w - - 0 1"); /* drawn pawn endgame except mine doesn't draw it */
    // decode_fen("8/4k3/8/8/5K2/5P2/8/8 w - - 0 1"); /* winning pawn endgame */
    // decode_fen("7k/5P2/5K2/7P/1p6/1P6/8/8 b - - 0 1"); /* rook promotion */
    // decode_fen("8/2q1P1k1/8/5K2/8/8/5B2/8 w - - 0 1"); /* knight promotion + bishop knight mate */
    // decode_fen("8/7p/8/7k/8/2NN4/2K5/8 w - - 0 1"); /* checkmate with 2 knights */
    // decode_fen("k2K4/8/8/8/8/8/8/8 w - - 0 1"); /* lone kings */

    decode_fen("7k/6q1/8/8/3p4/8/4P3/1K6 b - - 0 1"); /* testing new check detection */

    // decode_fen("5b1k/2p3pp/1p6/4QpN1/3P1P2/6K1/2nq2PP/8 w - - 0 1"); /* screwed this up against comm (play Qe6) */

    move_t move;
    vector<move_t> legal_moves;
    generate_moves(&legal_moves);
 
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
                            game_history.erase(b.board_hash);
                            unmake_move(LAST_MOVE(b.state_history.top()));
                            LoadDisplayBoardFromGameState(b.sq_board);
                            generate_moves(&legal_moves);
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
            generate_moves(&legal_moves);
            if(legal_moves.size() == 0) {
                if(checking_pieces()) {
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
            generate_moves(&legal_moves);

            madeMove = false;
        }

        /* code to play itself */
        // LoadDisplayBoardFromGameState(b.sq_board);
        // SDL_RenderClear(renderer);
        // DrawChessBoard();
        // DrawPieces();
        // DrawSelectedPiece(selectedPiece);
        // SDL_RenderPresent(renderer);

        // legal_moves.clear();
        // generate_moves(&legal_moves);
        // if(legal_moves.size() == 0) {
        //     if(checking_pieces()) {
        //         cout << "Checkmate!" << endl;
        //         break;
        //     }
        //     cout << "Stalemate!" << endl;
        //     break;
        // }

        // move = find_best_move();
        // make_move(move); // leaking memory here
        // LoadDisplayBoardFromGameState(b.sq_board);

        // legal_moves.clear();
        // generate_moves(&legal_moves);
        /* code to play itself */

        SDL_RenderClear(renderer);
        DrawChessBoard();
        DrawPieces();
        DrawSelectedPiece(selectedPiece);
        SDL_RenderPresent(renderer);

        if(legal_moves.size() == 0) {
            if(checking_pieces()) {
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

/*
    Notes:
        - Load the display board with rects when you decode the fen
        - draw the board based off of the display board

    - there is some strange bug with this position (move white rook):
        8/8/8/8/8/2K5/7R/1k6 w - - 0 1
      when the computer has a low number of moves, it just doesn't play a move...
      its finding my move to be the best move?
      its not my move generation that's wrong, its my find_best_move function

      it thinks that they all lead to mate eventually, so it doesn't know which to pick
      It can't see far enough to know the exact mate

      To do list:
       - dark squares are light squares
       - zobrist hashing and repetition draws (bookmark)
       - test simple checkmates again
       - transposition table
       - opening book
       - iterative deepening
       - better move ordering
       - qsearch (done but now its very slow... try move ordering?)
       - better eval function (done for now)
       - transposition tables
       - eventually switch away from SDL for better quality... Maybe try Unity?
         or UE4?
       - store the material in the board state
       - add checks to qsearch


      before adding new eval, we have 3527924 positions searched after Nd5

      consider doing what I do in evaluate() incrementally in make_move
      then just access the evaluation of the board when evaluating

      now I would like to make a better in_check function. This will consider
      the opponent's last move, and see if the piece they moved is attacking
      the king. Also I need to make a ray between where the piece came from,
      and see if that intersects with an opponent sliding piece

      outline for function
      1. check to see if the last move was no move
            if so, get the number of attackers in the traditional way
      2. look at their last move and see the piece that they just moved.
      Get the attacks of that piece from that square and see if it attacks
      our king. Then use the ray functions to get the ray from the square 
      that their piece came from and see if they are in check

      test this function a lot for all types of checks

      I can use this function in place of the move generation and for when 
      im searching
*/