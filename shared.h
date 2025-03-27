/* ========== shared.h ========== */
#ifndef SHARED_H
#define SHARED_H

#include <SDL.h>
#include <SDL_net.h>

#define MAX_PLAYERS 4
#define PORT 1234
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define RECT_SIZE 50

// Simple message types
typedef enum {
    MSG_MOVE = 0,       // dx, dy movement
    MSG_KEEPALIVE = 1,  // not used in this demo, but an example
    MSG_DISCONNECT = 2  // client signals it is disconnecting
} MessageType;

typedef struct {
    int messageType;  // One of MessageType
    int playerID;     // Which player is sending
    int dx;           // Movement X
    int dy;           // Movement Y
    int health;       // Just an example extra field
} PacketData;

#endif
