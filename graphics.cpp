#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "engine.h"

using namespace std;

#define SCREEN_WIDTH 1080 
#define SCREEN_HEIGHT 1080

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
                SDL_SetRenderDrawColor(renderer, 118, 150, 86, 0xFF);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 238, 238, 210, 0xFF);
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
                SDL_RenderCopy(renderer, Piece_Textures[index_from_piece(display_square.pc)], NULL, &(display_square.rect));
            }
        }
    }
    
}

void DrawSelectedPiece(mv_piece_t selectedPiece) {
    if(selectedPiece.pc != EMPTY) {
        SDL_RenderCopy(renderer, Piece_Textures[index_from_piece(selectedPiece.pc)], NULL, &(selectedPiece.rect));
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
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("Error: SDL failed to initialize\nSDL Error: '%s'\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Chess", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
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

    // board_t *board = decode_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    // board_t *board = decode_fen("8/3r4/3k4/8/8/3K4/8/8 w - - 0 1");
    board_t *board = decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    stack<board_t *> game; // add functionality for going back
    move_t move;
    vector<move_t> legal_moves;
    generate_moves(board, &legal_moves);

    LoadDisplayBoardFromGameState(board->sq_board);
    
    mv_piece_t selectedPiece;
    selectedPiece.pc = EMPTY;

    bool leftMouseButtonDown = false;
    bool madeMove = false;
    SDL_Point mousePos;
    bool running = true;
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
                            square target = SquareNumFromLoc(mousePos);
                            move.tar_piece = d_sq->pc;
                            move.target = target;
                            if(move.mv_piece == (WHITE | PAWN) && (target / 8) == 7) {
                                move.promotion_piece = WHITE | QUEEN;
                            }
                            else if(move.mv_piece == (BLACK | PAWN) && (target / 8) == 0) {
                                move.promotion_piece = BLACK | QUEEN;
                            }
                            else{
                                move.promotion_piece = EMPTY;
                            }

                            for (move_t legal_move : legal_moves) {
                                if(move.start == legal_move.start &&
                                   move.target == legal_move.target) {
                                    board = make_move(board, move);
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
                        square start = SquareNumFromLoc(mousePos);
                        if(d_sq->pc != EMPTY) {
                            selectedPiece.pc = d_sq->pc; // set whats inside equal to each other
                            selectedPiece.start_sq = start;
                            selectedPiece.rect = d_sq->rect;
                            move.mv_piece = d_sq->pc;
                            move.start = start;
                            d_sq->pc = EMPTY;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        if(madeMove) {
            LoadDisplayBoardFromGameState(board->sq_board);
            SDL_RenderClear(renderer);
            DrawChessBoard();
            DrawPieces();
            DrawSelectedPiece(selectedPiece);
            SDL_RenderPresent(renderer);

            move = find_best_move(board);
            board = make_move(board, move); // leaking memory here
            LoadDisplayBoardFromGameState(board->sq_board);

            legal_moves.clear();
            generate_moves(board, &legal_moves);

            // cout << "Player moves: " << legal_moves.size() << endl;

            // cout << notation_from_move(move, legal_moves, board);
            // cout << endl;

            madeMove = false;
        }
        SDL_RenderClear(renderer);
        DrawChessBoard();
        DrawPieces();
        DrawSelectedPiece(selectedPiece);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    free(board);
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
       - opening book
       - move ordering
       - qsearch (done but now its very slow... try move ordering?)
       - better eval function (done for now)
       - transposition tables
       - eventually switch away from SDL for better quality... Maybe try Unity?
         or UE4?
       - store the material in the board state


      before adding new eval, we have 3527924 positions searched after Nd5
*/