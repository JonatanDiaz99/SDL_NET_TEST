#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_net.h>

#define PORT 1234
#define MAX_PLAYERS 2
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct {
    int id;   // Player ID
    int x;
    int y;
} Player;

Player players[MAX_PLAYERS];
int connectedPlayers = 0;

// Store the IP/port of each connected client
IPaddress clientAddrs[MAX_PLAYERS];
UDPsocket serverSocket = NULL;
UDPpacket *recvPacket = NULL;
UDPpacket *sendPacket = NULL;

// Attempt to find which player (if any) is associated with this address
int getPlayerIDByAddress(IPaddress addr) {
    for (int i = 0; i < connectedPlayers; i++) {
        if (clientAddrs[i].host == addr.host && clientAddrs[i].port == addr.port) {
            return i;  // Found existing player
        }
    }
    return -1; // Unknown address
}

// Broadcast the entire players[] array to all connected clients
void sendPlayerPositions() {
    if (!sendPacket) return;

    // Copy the entire array of Player structs into the packet
    memcpy(sendPacket->data, players, sizeof(players));
    sendPacket->len = sizeof(players);

    // Send to each connected player
    for (int i = 0; i < connectedPlayers; i++) {
        sendPacket->address = clientAddrs[i];
        SDLNet_UDP_Send(serverSocket, -1, sendPacket);
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (SDL_Init(0) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (SDLNet_Init() < 0) {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        SDL_Quit();
        return 1;
    }

    // Open a UDP socket on the specified port
    serverSocket = SDLNet_UDP_Open(PORT);
    if (!serverSocket) {
        printf("Failed to open UDP socket on port %d: %s\n", PORT, SDLNet_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Allocate packets for receiving and sending data
    recvPacket = SDLNet_AllocPacket(512);
    sendPacket = SDLNet_AllocPacket(512);
    if (!recvPacket || !sendPacket) {
        printf("Failed to allocate UDP packets.\n");
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize player data (positions, IDs)
    players[0].id = 0;  players[0].x = 100;  players[0].y = 250;
    players[1].id = 1;  players[1].x = 200;  players[1].y = 250;

    connectedPlayers = 0; // No one connected yet

    printf("UDP Server is running on port %d...\n", PORT);

    // Main server loop
    while (1) {
        // Check if a packet has arrived
        while (SDLNet_UDP_Recv(serverSocket, recvPacket)) {
            // Figure out who sent this packet
            IPaddress sender = recvPacket->address;
            int senderID = getPlayerIDByAddress(sender);

            // If we don't know this sender yet, try to add them
            if (senderID < 0 && connectedPlayers < MAX_PLAYERS) {
                senderID = connectedPlayers;
                clientAddrs[senderID] = sender;
                connectedPlayers++;

                printf("New player connected with ID %d (host=%u, port=%u)\n",
                       senderID, sender.host, sender.port);
            }

            // If we have a valid senderID, parse the incoming data
            if (senderID >= 0 && senderID < MAX_PLAYERS) {
                if (recvPacket->len >= 2 * (Uint16)sizeof(int)) {
                    // Interpret the received data as two ints: dx, dy
                    int dx, dy;
                    memcpy(&dx, recvPacket->data, sizeof(int));
                    memcpy(&dy, (char*)recvPacket->data + sizeof(int), sizeof(int));

                    // Update that player's position
                    players[senderID].x += dx;
                    players[senderID].y += dy;
                    
                    // Simple boundary check (optional)
                    if (players[senderID].x < 0) players[senderID].x = 0;
                    if (players[senderID].x > SCREEN_WIDTH) players[senderID].x = SCREEN_WIDTH;
                    if (players[senderID].y < 0) players[senderID].y = 0;
                    if (players[senderID].y > SCREEN_HEIGHT) players[senderID].y = SCREEN_HEIGHT;
                }
            }
        }

        // Broadcast updated positions to all players
        if (connectedPlayers > 0) {
            sendPlayerPositions();
        }

        // Limit loop frequency (~60 FPS)
        SDL_Delay(16);
    }

    // Cleanup (technically never reached in this code)
    SDLNet_FreePacket(recvPacket);
    SDLNet_FreePacket(sendPacket);
    SDLNet_UDP_Close(serverSocket);

    SDLNet_Quit();
    SDL_Quit();
    return 0;
}
