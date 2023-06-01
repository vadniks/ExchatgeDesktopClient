
#include <sdl_net/SDL_net.h>
#include "crypto.h"
#include "net.h"

#define STATE(x, y) static const int STATE_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(SERVER_PUBLIC_KEY_RECEIVED, 1)
STATE(CLIENT_PUBLIC_KEY_SENT, 2)
STATE(NONCE_RECEIVED, 3)
STATE(READY, STATE_NONCE_RECEIVED)

THIS(
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    int state;
)

static void initiateSecuredConnection() {
    const unsigned publicKeySize = crPublicKeySize(),
        charSize = sizeof(char),
        nonceSize = crNonceSize();

    byte* serverPublicKey = SDL_calloc(publicKeySize, charSize);
    SDLNet_TCP_Recv(this->socket, serverPublicKey, (int) publicKeySize);
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    byte* clientPublicKey = crInit(serverPublicKey);
    if (!clientPublicKey) return;

    SDLNet_TCP_Send(this->socket, clientPublicKey, (int) publicKeySize);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;

    byte* nonce = SDL_calloc(nonceSize, charSize);
    SDLNet_TCP_Recv(this->socket, nonce, (int) nonceSize);
    crSetNonce(nonce);
    this->state = STATE_NONCE_RECEIVED;
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

    initiateSecuredConnection();
    return this->state == STATE_READY;
}

static bool isDataAvailable() {
    return SDLNet_CheckSockets(this->socketSet, 0) == 1
        && SDLNet_SocketReady(this->socket) != 0;
}

static message* unpackMessage(byte* buffer) {
    message* msg = SDL_malloc(sizeof *msg);
    unsigned intSize = sizeof(int), longSize = sizeof(long);

    SDL_memcpy(&(msg->flag), buffer, intSize);
    SDL_memcpy(&(msg->timestamp), buffer, longSize);
    SDL_memcpy(&(msg->size), buffer, intSize);
    SDL_memcpy(&(msg->index), buffer, intSize);
    SDL_memcpy(&(msg->count), buffer, intSize);
    SDL_memcpy(&(msg->body), buffer + NET_MESSAGE_HEAD_SIZE, NET_MESSAGE_BODY_SIZE);

    SDL_free(buffer);
    return msg;
}

static byte* packMessage(message* msg) {
    byte* buffer = SDL_calloc(NET_RECEIVE_BUFFER_SIZE, sizeof(char));
    unsigned intSize = sizeof(int), longSize = sizeof(long);

    SDL_memcpy(buffer, &(msg->flag), intSize);
    SDL_memcpy(buffer + intSize, &(msg->timestamp), longSize);
    SDL_memcpy(buffer + intSize + longSize, &(msg->size), intSize);
    SDL_memcpy(buffer + intSize * 2 + longSize, &(msg->index), intSize);
    SDL_memcpy(buffer + intSize * 3 + longSize, &(msg->count), intSize);

    SDL_free(msg);
    return buffer;
}

static bool test = false; // TODO: test only
void ntListen() {
    message* msg = NULL;

    if (isDataAvailable()) {
        byte* buffer = SDL_calloc(NET_RECEIVE_BUFFER_SIZE, sizeof(char));

        SDLNet_TCP_Recv(this->socket, buffer, NET_RECEIVE_BUFFER_SIZE);
        msg = unpackMessage(buffer);
    }

    switch (this->state) {
        case STATE_READY:
            if (test) break;
            test = true;

            ntSend((byte*) "Hello World!", 12); // TODO: test only
            SDL_Log("hello world sent");

            break; // TODO
    }

    SDL_free(msg);
}

void ntSend(byte* message, unsigned size) {
    byte* encrypted = crEncrypt(message, size);
//    SDL_free(message);
    if (!encrypted) return;

    SDLNet_TCP_Send(this->socket, encrypted, (int) size);
    SDL_free(encrypted);
}

void ntClean() {
    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();
    SDL_free(this);
}
