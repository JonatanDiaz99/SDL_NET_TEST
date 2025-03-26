#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_net.h>

#define PORT 1234
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define RECT_SIZE 50
#define MAX_PLAYERS 2

typedef struct {
    int id;
    int x, y;
} Player;

Player players[MAX_PLAYERS];  // Two players
TCPsocket clientSockets[MAX_PLAYERS];
int connectedPlayers = 0;

void sendPlayerPositions() {
    for (int i = 0; i < connectedPlayers; i++) {
        for (int j = 0; j < MAX_PLAYERS; j++) {
            SDLNet_TCP_Send(clientSockets[i], &players[j], sizeof(Player));
        }
    }
}

int main() {
    if (SDL_Init(0) < 0 || SDLNet_Init() < 0) {
        printf("Failed to initialize: %s\n", SDLNet_GetError());
        return 1;
    }

    IPaddress ip;
    TCPsocket serverSocket;

    if (SDLNet_ResolveHost(&ip, NULL, PORT) < 0 || !(serverSocket = SDLNet_TCP_Open(&ip))) {
        printf("Server creation failed: %s\n", SDLNet_GetError());
        return 1;
    }

    printf("Server running on port %d...\n", PORT);

    // Initialize player positions
    players[0] = (Player){0, 100, 250};
    players[1] = (Player){1, 200, 250};

    while (connectedPlayers < MAX_PLAYERS) {
        TCPsocket client = SDLNet_TCP_Accept(serverSocket);
        if (client) {
            clientSockets[connectedPlayers] = client;
            SDLNet_TCP_Send(client, &connectedPlayers, sizeof(int));  // Send player ID
            printf("Player %d connected!\n", connectedPlayers);
            connectedPlayers++;
        }
    }

    while (1) {
        for (int i = 0; i < connectedPlayers; i++) {
            int dx = 0, dy = 0;
            int recvStatus = SDLNet_TCP_Recv(clientSockets[i], &dx, sizeof(int));
            recvStatus += SDLNet_TCP_Recv(clientSockets[i], &dy, sizeof(int));

            if (recvStatus > 0) {
                players[i].x += dx;
                players[i].y += dy;
            }
        }

        sendPlayerPositions();
        SDL_Delay(16);  // 60 FPS limit
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        SDLNet_TCP_Close(clientSockets[i]);
    }
    SDLNet_TCP_Close(serverSocket);
    SDLNet_Quit();
    SDL_Quit();

    return 0;
}