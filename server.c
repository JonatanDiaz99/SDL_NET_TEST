/* ========== server.c ========== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_net.h>

#include "shared.h"

// Each player data: position on screen
typedef struct {
    int x;
    int y;
} PlayerState;

static PlayerState players[MAX_PLAYERS];
static IPaddress clients[MAX_PLAYERS];
static int connectedPlayers = 0;

// Helper: find which player ID corresponds to an IP/port
// Return -1 if not found
int findClient(IPaddress addr) {
    for (int i = 0; i < connectedPlayers; i++) {
        if (clients[i].host == addr.host && clients[i].port == addr.port) {
            return i;
        }
    }
    return -1;
}

// Remove client by index
void removeClient(int index) {
    printf("Removing client at index %d\n", index);
    // Shift everything down from the end
    for (int i = index; i < connectedPlayers - 1; i++) {
        clients[i] = clients[i + 1];
        players[i] = players[i + 1]; // Also shift position
    }
    connectedPlayers--;
}

// Broadcast all players' positions to each client
void broadcastPlayers(UDPsocket sock, UDPpacket *packet) {
    // We’ll send the entire players[] array in one go
    // or we can send multiple messages, but let's do one shot.
    // We'll just do: 2 * int for each player: x, y
    // But let's do it very simply with a second buffer or struct.

    // We'll create a temporary buffer: each player is 2 ints => 8 bytes
    // With MAX_PLAYERS players, that's 8 * MAX_PLAYERS = 32 for 4 players,
    // but let's be safe and assume 8 * 16 or just 128.
    // For clarity, let's do a small known limit.
    char buffer[512];
    int offset = 0;

    // Copy connectedPlayers (so client knows how many are valid)
    memcpy(buffer + offset, &connectedPlayers, sizeof(int));
    offset += sizeof(int);

    // Now copy positions for each player slot in the first connectedPlayers
    for (int i = 0; i < connectedPlayers; i++) {
        memcpy(buffer + offset, &players[i].x, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &players[i].y, sizeof(int));
        offset += sizeof(int);
    }

    // Put this in the outgoing packet
    memcpy(packet->data, buffer, offset);
    packet->len = offset;

    // Send to each connected client
    for (int i = 0; i < connectedPlayers; i++) {
        packet->address = clients[i];
        SDLNet_UDP_Send(sock, -1, packet);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv; // Silence unused warnings if you like

    if (SDL_Init(0) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (SDLNet_Init() < 0) {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        SDL_Quit();
        return 1;
    }

    // Open server UDP socket
    UDPsocket serverSocket = SDLNet_UDP_Open(PORT);
    if (!serverSocket) {
        printf("Failed to open UDP socket on port %d\n", PORT);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Packets for sending/receiving
    UDPpacket *recvPacket = SDLNet_AllocPacket(512);
    UDPpacket *sendPacket = SDLNet_AllocPacket(512);
    if (!recvPacket || !sendPacket) {
        printf("Failed to allocate packets\n");
        SDLNet_UDP_Close(serverSocket);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize player positions
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].x = 100 + i * 50;
        players[i].y = 100 + i * 50;
    }

    printf("UDP server started on port %d\n", PORT);

    bool running = true;
    while (running) {
        // Check for packets
        while (SDLNet_UDP_Recv(serverSocket, recvPacket)) {
            // We expect a PacketData or a smaller/larger message
            if (recvPacket->len == sizeof(PacketData)) {
                PacketData pkg;
                memcpy(&pkg, recvPacket->data, sizeof(PacketData));

                // Find or add the sender
                IPaddress sender = recvPacket->address;
                int playerIndex = findClient(sender);

                // If unknown, add if we have capacity
                if (playerIndex == -1 && connectedPlayers < MAX_PLAYERS) {
                    playerIndex = connectedPlayers;
                    clients[playerIndex] = sender;
                    connectedPlayers++;
                    printf("New client added as player %d\n", playerIndex);
                }

                if (playerIndex >= 0) {
                    // Process the incoming message
                    if (pkg.messageType == MSG_MOVE) {
                        // Adjust that player's position
                        players[playerIndex].x += pkg.dx;
                        players[playerIndex].y += pkg.dy;

                        // Just clamp to screen boundaries
                        if (players[playerIndex].x < 0) players[playerIndex].x = 0;
                        if (players[playerIndex].x > SCREEN_WIDTH) players[playerIndex].x = SCREEN_WIDTH;
                        if (players[playerIndex].y < 0) players[playerIndex].y = 0;
                        if (players[playerIndex].y > SCREEN_HEIGHT) players[playerIndex].y = SCREEN_HEIGHT;
                    }
                    else if (pkg.messageType == MSG_DISCONNECT) {
                        // This client is disconnecting
                        printf("Player %d disconnected.\n", playerIndex);
                        removeClient(playerIndex);

                        if (connectedPlayers == 0) {
                            printf("All clients disconnected. Shutting down server.\n");
                            running = false;
                            break;
                        }
                    }
                }
            }
        }

        // Broadcast updated positions to all clients
        if (connectedPlayers > 0) {
            broadcastPlayers(serverSocket, sendPacket);
        }

        // Minimal delay
        SDL_Delay(16);

        if (!running && connectedPlayers == 0) {
            // break out of outer loop
            break;
        }
    }

    SDLNet_FreePacket(recvPacket);
    SDLNet_FreePacket(sendPacket);
    SDLNet_UDP_Close(serverSocket);
    SDLNet_Quit();
    SDL_Quit();
    return 0;
}
