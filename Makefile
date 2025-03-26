# Compiler
CC=gcc

# Compiler flags
CFLAGS= -g -Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LDFLAGS= $(shell sdl2-config --libs) -lSDL2_net -lSDL2_ttf -lSDL2_image -lSDL2_mixer

# Targets
SERVER_TARGET=server
CLIENT_TARGET=client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER_TARGET) $(LDFLAGS)

$(CLIENT_TARGET): client.c
	$(CC) $(CFLAGS) client.c -o $(CLIENT_TARGET) $(LDFLAGS)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

run_server:
	./$(SERVER_TARGET)

run_client:
	./$(CLIENT_TARGET)

run_clients:
	./$(CLIENT_TARGET) & ./$(CLIENT_TARGET)
