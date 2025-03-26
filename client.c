#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_net.h>

#define PORT 1234
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define RECT_SIZE 50
#define CENTER_X (SCREEN_WIDTH / 2 - RECT_SIZE / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2 - RECT_SIZE / 2)


typedef struct {
    int id;
    int x, y;
} Player;

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDLNet_Init() < 0) {
        printf("Failed to initialize: %s\n", SDLNet_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL Net Multiplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    IPaddress ip;
    TCPsocket clientSocket;
    if (SDLNet_ResolveHost(&ip, "127.0.0.1", PORT) < 0 || !(clientSocket = SDLNet_TCP_Open(&ip))) {
        printf("Failed to connect to server: %s\n", SDLNet_GetError());
        return 1;
    }

    int playerID;
    SDLNet_TCP_Recv(clientSocket, &playerID, sizeof(int));  // Receive assigned player ID
    printf("Connected as Player %d\n", playerID);

    Player players[2];
    int running = 1;
    SDL_Event event;

    while (running) {
        int dx = 0, dy = 0;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: dy = -5; break;
                    case SDLK_DOWN: dy = 5; break;
                    case SDLK_LEFT: dx = -5; break;
                    case SDLK_RIGHT: dx = 5; break;
                }
            }
        }

        SDLNet_TCP_Send(clientSocket, &dx, sizeof(int));
        SDLNet_TCP_Send(clientSocket, &dy, sizeof(int));

        for (int i = 0; i < 2; i++) {
            SDLNet_TCP_Recv(clientSocket, &players[i], sizeof(Player));
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect rect1 = {players[0].x, players[0].y, RECT_SIZE, RECT_SIZE};
        SDL_RenderFillRect(renderer, &rect1);

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect rect2 = {players[1].x, players[1].y, RECT_SIZE, RECT_SIZE};
        SDL_RenderFillRect(renderer, &rect2);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDLNet_TCP_Close(clientSocket);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDLNet_Quit();
    SDL_Quit();

    return 0;
}
