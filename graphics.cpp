#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "engine.h"

using namespace std;

#define SCREEN_WIDTH 1080 
#define SCREEN_HEIGHT 1080

#define BB_PATH "assets/bB.png"
#define BK_PATH "assets/bK.png"
#define BN_PATH "assets/bN.png"
#define BP_PATH "assets/bP.png"
#define BQ_PATH "assets/bQ.png"
#define BR_PATH "assets/bR.png"
#define WB_PATH "assets/wB.png"
#define WK_PATH "assets/wK.png"
#define WN_PATH "assets/wN.png"
#define WP_PATH "assets/wP.png"
#define WQ_PATH "assets/wQ.png"
#define WR_PATH "assets/wR.png"

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
    Piece_Textures[BLACK_BISHOPS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(BB_PATH));
    Piece_Textures[BLACK_KINGS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(BK_PATH));
    Piece_Textures[BLACK_KNIGHTS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(BN_PATH));
    Piece_Textures[BLACK_PAWNS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(BP_PATH));
    Piece_Textures[BLACK_QUEENS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(BQ_PATH));
    Piece_Textures[BLACK_ROOKS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(BR_PATH));

    Piece_Textures[WHITE_BISHOPS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(WB_PATH));
    Piece_Textures[WHITE_KINGS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(WK_PATH));
    Piece_Textures[WHITE_KNIGHTS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(WN_PATH));
    Piece_Textures[WHITE_PAWNS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(WP_PATH));
    Piece_Textures[WHITE_QUEENS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(WQ_PATH));
    Piece_Textures[WHITE_ROOKS_INDEX] = SDL_CreateTextureFromSurface(renderer, IMG_Load(WR_PATH));
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

    LoadPieceTextures();

    board_t *board = decode_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ");
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
                
                            // d_sq->pc = selectedPiece.pc;
                            // instead of doing this, this should generate a move which will
                            // be applied to the board, and then the board will be reloaded from game state
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
            board = make_move(board, find_best_move(board, 5));

            legal_moves.clear();
            generate_moves(board, &legal_moves);
            LoadDisplayBoardFromGameState(board->sq_board);
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

    return 0;
}

/*
    Notes:
        - Load the display board with rects when you decode the fen
        - draw the board based off of the display board

*/