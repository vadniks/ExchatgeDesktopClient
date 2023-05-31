
#include <sdl_net/SDL_net.h>
#include "crypto.h"
#include "net.h"

#define STATE(x, y) static const int STATE_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(OBTAINED_SERVER_PUBLIC_KEY, 1)
STATE(SENT_CLIENT_PUBLIC_KEY, 2)

THIS(
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    int state;
)

static byte* fetchServerPublicKey() {
    byte* serverPublicKey = SDL_malloc(crPublicKeySize() * sizeof(char));
    // TODO
    return serverPublicKey;
}

bool ntInit() {
    this = SDL_malloc(sizeof *this);
    SDLNet_Init();

    IPaddress address;
    SDLNet_ResolveHost(&address, NET_HOST, NET_PORT);

    this->socket = SDLNet_TCP_OpenClient(&address);
    if (!this->socket) exit(1);

    this->socketSet = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(this->socketSet, this->socket);

    crInit(fetchServerPublicKey());
    return false;
}

static bool isDataAvailable() {
    return SDLNet_CheckSockets(this->socketSet, 0) == 1
        && SDLNet_SocketReady(this->socket) != 0;
}

void ntListen() {
    if (!isDataAvailable()) return;

    byte* buffer = SDL_calloc(NET_RECEIVE_BUFFER_SIZE, sizeof(char));
    SDLNet_TCP_Recv(this->socket, buffer, NET_RECEIVE_BUFFER_SIZE);
    // TODO
}

void ntSend(byte* message) {
    byte* encrypted = crEncrypt(message);
    SDL_free(message);

    // TODO
}

byte* ntReceive() {
    byte* message = NULL; // TODO
    byte* decrypted = crDecrypt(message);
    SDL_free(message);

    return NULL;
}

void ntClean() {
    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();
    SDL_free(this);
}
