
#include <sdl_net/SDL_net.h>
#include "crypto.h"
#include "net.h"

#define STATE(x, y) static const int STATE_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(SECURED_CONNECTION_INITIATED, 1)

THIS(
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    int state;
)

bool ntInit() {
    this = SDL_malloc(sizeof *this);
    SDLNet_Init();

    IPaddress address;
    SDLNet_ResolveHost(&address, NET_HOST, NET_PORT);

    this->socket = SDLNet_TCP_OpenClient(&address);
    if (!this->socket) exit(1);

    this->socketSet = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(this->socketSet, this->socket);

    return false;
}

static bool isDataAvailable() {
    return SDLNet_CheckSockets(this->socketSet, 0) == 1
        && SDLNet_SocketReady(this->socket) != 0;
}

static void initiateSecuredConnection(byte* body) {
    const unsigned publicKeySize = crPublicKeySize();

    byte* serverPublicKey = SDL_calloc(publicKeySize, sizeof(char));
    SDL_memcpy(serverPublicKey, body, publicKeySize);

    byte* clientPublicKey = crInit(serverPublicKey);
    SDLNet_TCP_Send(this->socket, clientPublicKey, (int) publicKeySize);

    SDL_Log("key sent"); // TODO: test only
    this->state = STATE_SECURED_CONNECTION_INITIATED;
}

static byte* receiveBufferToMessageBody(byte* buffer) {
    byte* body = SDL_calloc(NET_MESSAGE_BODY_SIZE, sizeof(char));
    SDL_memcpy(body, buffer + NET_MESSAGE_HEAD_SIZE, NET_MESSAGE_BODY_SIZE);
    return body;
}

static int receiveBufferToMessageHead(byte* buffer) {
    int head = 0;
    SDL_memcpy(&head, buffer, NET_MESSAGE_HEAD_SIZE);
    return head;
}

static bool test = false; // TODO: test only
void ntListen() {
//    int head = 0;
//    byte* body = NULL;
    byte* buffer = NULL;

    if (isDataAvailable()) {
        /*byte**/ buffer = SDL_calloc(NET_RECEIVE_BUFFER_SIZE, sizeof(char));
        SDLNet_TCP_Recv(this->socket, buffer, NET_RECEIVE_BUFFER_SIZE);

//        head = receiveBufferToMessageHead(buffer); // TODO: implement message sections splitting on the server
//        body = receiveBufferToMessageBody(buffer);
//        SDL_free(buffer);
    }

    switch (this->state) {
        case STATE_DISCONNECTED: initiateSecuredConnection(buffer/*body*/); break;
        case STATE_SECURED_CONNECTION_INITIATED:
            if (test) break;
            test = true;

            ntSend((byte*) "Hello World!", 12); // TODO: test only
            SDL_Log("hello world sent");

            break; // TODO
    }
}

void ntSend(byte* message, unsigned size) {
    byte* encrypted = crEncrypt(message, size);
//    SDL_free(message);
    if (!encrypted) return;

    SDLNet_TCP_Send(this->socket, encrypted, (int) size);
    SDL_free(encrypted);
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
