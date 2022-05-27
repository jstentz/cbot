#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

#include "engine.h"

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

SDL_Window *window;
SDL_Renderer *renderer;

int sq_width = (SCREEN_WIDTH > SCREEN_HEIGHT) ? (SCREEN_HEIGHT / 8) : (SCREEN_WIDTH / 8);

SDL_Surface *Piece_Textures[12];

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

// void Init_Pieces(SDL_Renderer *renderer) {
//     Piece_Textures[0] = IMG_LoadTexture(renderer);
// }

// void DrawPieces() {

// }

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

    Piece_Textures[0] = IMG_Load(BB_PATH);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, Piece_Textures[0]);

    bool running = true;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    running = false;
                    break;

                default:
                    break;
            }
        }

        // SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
        SDL_RenderClear(renderer);
        DrawChessBoard();
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}