
#include <sdl_net/SDL_net.h>
#include "crypto.h"
#include "net.h"

typedef struct {
    byte* publicKey;
} this_t;

static this_t* this = NULL;

char* net() { // TODO: left as an example, will be removed
    IPaddress address;
    SDLNet_ResolveHost(&address, "127.0.0.1", 8080);

    char* buffer = NULL;

    TCPsocket socket = SDLNet_TCP_OpenClient(&address);
    if (!socket) goto end;

    SDLNet_TCP_Send(socket, "Hello", 5);

    buffer = SDL_calloc(256, sizeof(char));
    SDLNet_TCP_Recv(socket, buffer, 256);
    SDL_Log("%s\n", buffer);

    end:
    SDLNet_TCP_Close(socket);
    return buffer;
}

static unsigned char* fetchPublicKey() {
    return NULL;
}

bool ntInit() {
    this = SDL_calloc(1024/*TODO*/, sizeof(byte));
    this->publicKey = fetchPublicKey();
    return false;
}

void ntSend(byte* message) {

}

byte* ntReceive() {
    return NULL;
}

void ntClean() {
    SDL_free(this->publicKey);
}
