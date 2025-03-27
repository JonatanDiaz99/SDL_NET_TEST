/* ========== client.c ========== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_net.h>

#include "shared.h"

static UDPsocket udpSocket = NULL;
static UDPpacket *outPacket = NULL;
static UDPpacket *inPacket = NULL;

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (SDLNet_Init() < 0) {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a window
    SDL_Window *window = SDL_CreateWindow("UDP Client",
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
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Resolve the server address (change IP if needed)
    IPaddress serverAddr;
    if (SDLNet_ResolveHost(&serverAddr, "127.0.0.1", PORT) < 0) {
        printf("SDLNet_ResolveHost failed: %s\n", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Open a local UDP port (0 picks any available)
    udpSocket = SDLNet_UDP_Open(0);
    if (!udpSocket) {
        printf("SDLNet_UDP_Open failed: %s\n", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Allocate packets
    outPacket = SDLNet_AllocPacket(512);
    inPacket  = SDLNet_AllocPacket(512);
    if (!outPacket || !inPacket) {
        printf("Failed to allocate packets.\n");
        if (outPacket) SDLNet_FreePacket(outPacket);
        if (inPacket) SDLNet_FreePacket(inPacket);
        SDLNet_UDP_Close(udpSocket);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // We'll store up to MAX_PLAYERS positions
    int numPlayers = 0; 
    int playerPosX[MAX_PLAYERS];
    int playerPosY[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) {
        playerPosX[i] = 0;
        playerPosY[i] = 0;
    }
    bool keys[SDL_NUM_SCANCODES] ={0};
    bool running = true;
    while (running) {
        // Handle input
        SDL_Event event;
        int dx = 0, dy = 0;
    while (SDL_PollEvent(&event)){
        switch (event.type){
        case SDL_QUIT:
            running = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            keys[event.button.button] = SDL_PRESSED;
            break;
        case SDL_MOUSEBUTTONUP:
            keys[event.button.button] = SDL_RELEASED;
            break;
        case SDL_KEYDOWN:
            keys[event.key.keysym.scancode]  = true;
            break;
        case SDL_KEYUP:
            keys[event.key.keysym.scancode]  = false;
            break;
        default:
            break;
        }
    }
        if(keys[SDL_SCANCODE_UP]) dy = -5;
        if(keys[SDL_SCANCODE_DOWN]) dy = 5;
        if(keys[SDL_SCANCODE_LEFT]) dx = -5;
        if(keys[SDL_SCANCODE_RIGHT]) dx = 5;
        // dy(-up/ner), dx(-left/rhift) 
        // If there was a movement, send it
        if (dx != 0 || dy != 0) {
            PacketData pkg;
            pkg.messageType = MSG_MOVE;
            pkg.playerID = 0; // Example: no real ID system on client side
            pkg.dx = dx;
            pkg.dy = dy;
            pkg.health = 100; // arbitrary

            memcpy(outPacket->data, &pkg, sizeof(PacketData));
            outPacket->len = sizeof(PacketData);
            outPacket->address = serverAddr;
            SDLNet_UDP_Send(udpSocket, -1, outPacket);
        }

        // Receive updated positions from server
        while (SDLNet_UDP_Recv(udpSocket, inPacket)) {
            // We’re sending an int for connectedPlayers, then x,y for each
            // So total length = sizeof(int) + 2*sizeof(int)*connectedPlayers
            // We can parse it accordingly
            char *buf = (char *)inPacket->data;
            int offset = 0;
            // Check we at least have an int
            if (inPacket->len >= (int)sizeof(int)) {
                memcpy(&numPlayers, buf + offset, sizeof(int));
                offset += sizeof(int);

                // For each player
                for (int i = 0; i < numPlayers; i++) {
                    if (offset + 2 * (int)sizeof(int) <= inPacket->len) {
                        memcpy(&playerPosX[i], buf + offset, sizeof(int));
                        offset += sizeof(int);
                        memcpy(&playerPosY[i], buf + offset, sizeof(int));
                        offset += sizeof(int);
                    }
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw each player
        for (int i = 0; i < numPlayers; i++) {
            // Just alternate color for fun
            if (i % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            }
            SDL_Rect rect = { playerPosX[i], playerPosY[i], RECT_SIZE, RECT_SIZE };
            SDL_RenderFillRect(renderer, &rect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // If we’re disconnecting, let server know
    {
        PacketData pkg;
        pkg.messageType = MSG_DISCONNECT;
        pkg.playerID = 0;
        pkg.dx = 0;
        pkg.dy = 0;
        pkg.health = 0;

        memcpy(outPacket->data, &pkg, sizeof(PacketData));
        outPacket->len = sizeof(PacketData);
        outPacket->address = serverAddr;
        SDLNet_UDP_Send(udpSocket, -1, outPacket);
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
