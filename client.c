#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_net.h>

#define PORT 1234
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define RECT_SIZE 50

// Same Player struct as on the server
typedef struct {
    int id;
    int x;
    int y;
} Player;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    // Initialize SDL and SDL_net
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (SDLNet_Init() < 0) {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        SDL_Quit();
        return 1;
    }

    // Create an SDL window and renderer
    SDL_Window* window = SDL_CreateWindow("UDP Client Example",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // ------------------------------------------------------
    // UDP Socket Setup
    // ------------------------------------------------------
    // Resolve the server address (change IP to match your server if not on localhost)
    IPaddress serverAddr;
    if (SDLNet_ResolveHost(&serverAddr, "127.0.0.1", PORT) < 0) {
        printf("SDLNet_ResolveHost failed: %s\n", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Open a UDP socket on any free local port
    UDPsocket udpSocket = SDLNet_UDP_Open(0);
    if (!udpSocket) {
        printf("SDLNet_UDP_Open failed: %s\n", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Allocate packets for sending/receiving
    UDPpacket* outPacket = SDLNet_AllocPacket(512);
    UDPpacket* inPacket  = SDLNet_AllocPacket(512);
    if (!outPacket || !inPacket) {
        printf("Failed to allocate UDP packets.\n");
        if (outPacket) SDLNet_FreePacket(outPacket);
        if (inPacket)  SDLNet_FreePacket(inPacket);
        SDLNet_UDP_Close(udpSocket);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Array to store positions of up to 2 players
    Player players[2];
    memset(players, 0, sizeof(players));

    printf("UDP Client started. Sending data to server...\n");

    // Main loop
    int running = 1;
    while (running) {
        // Handle input
        SDL_Event event;
        int dx = 0, dy = 0;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:    dy = -5; break;
                    case SDLK_DOWN:  dy =  5; break;
                    case SDLK_LEFT:  dx = -5; break;
                    case SDLK_RIGHT: dx =  5; break;
                    default: break;
                }
            }
        }

        // ------------------------------------------------------
        // Send (dx, dy) to the server
        // ------------------------------------------------------
        // Put dx, dy in the outgoing packet
        memcpy(outPacket->data, &dx, sizeof(int));
        memcpy(outPacket->data + sizeof(int), &dy, sizeof(int));
        outPacket->len = 2 * sizeof(int);
        outPacket->address = serverAddr;

        // Actually send the packet
        SDLNet_UDP_Send(udpSocket, -1, outPacket);

        // ------------------------------------------------------
        // Attempt to receive updated player positions from the server
        // ------------------------------------------------------
        while (SDLNet_UDP_Recv(udpSocket, inPacket)) {
            // inPacket->data should contain the array of Player structs
            if (inPacket->len == sizeof(players)) {
                memcpy(players, inPacket->data, sizeof(players));
            }
            // If your server sends different-sized packets or multiple packet types,
            // you may need logic to distinguish them here.
        }

        // ------------------------------------------------------
        // Render the scene
        // ------------------------------------------------------
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw player 0 in red
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect rect1 = { players[0].x, players[0].y, RECT_SIZE, RECT_SIZE };
        SDL_RenderFillRect(renderer, &rect1);

        // Draw player 1 in blue
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect rect2 = { players[1].x, players[1].y, RECT_SIZE, RECT_SIZE };
        SDL_RenderFillRect(renderer, &rect2);

        SDL_RenderPresent(renderer);

        // Limit CPU usage (~60 FPS)
        SDL_Delay(16);
    }

    // Cleanup
    SDLNet_FreePacket(outPacket);
    SDLNet_FreePacket(inPacket);
    SDLNet_UDP_Close(udpSocket);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDLNet_Quit();
    SDL_Quit();
    return 0;
}
