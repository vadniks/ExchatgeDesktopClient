
#include <sdl_net/SDL_net.h>
#include <assert.h>
#include "crypto.h"
#include "net.h"

#define STATE(x, y) static const int STATE_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(SERVER_PUBLIC_KEY_RECEIVED, 1)
STATE(CLIENT_PUBLIC_KEY_SENT, 2)
STATE(READY, STATE_CLIENT_PUBLIC_KEY_SENT) // TODO: check client signature on server and check server signature on client

THIS( // TODO: check client's authentication by token
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    int state;
)

static const int PORT = 8080;
static const unsigned MESSAGE_HEAD_SIZE = sizeof(int) * 4 + sizeof(long); // 24
static const unsigned MESSAGE_BODY_SIZE = 1 << 10; // 1024
static const unsigned MESSAGE_SIZE = MESSAGE_HEAD_SIZE + MESSAGE_BODY_SIZE; // 1048
static const unsigned PADDING_BLOCK_SIZE = 16;
static const int FLAG_UNAUTHENTICATED = 0x7ffffffe;
static const int FLAG_FINISH = 0x7fffffff;

typedef struct {
    // begin head
    int flag; // short service description of message
    unsigned long timestamp; // message created at
    unsigned size; // actual size of th payload
    unsigned index; // message part index if the whole message cannot fit in boy
    unsigned count; // total count of message parts
    // end head

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-folding-constant" // If this causes compile troubles declare this constant as a macro
    byte body[MESSAGE_BODY_SIZE]; // payload
#pragma clang diagnostic pop

} Message;

static unsigned receiveBufferSize() { return cryptoEncryptedSize(); }

static byte* packMessage(Message* msg); // TODO: test only

static void initiateSecuredConnection() {
    const unsigned publicKeySize = cryptoPublicKeySize(),
        charSize = sizeof(char);

    byte* serverPublicKey = SDL_calloc(publicKeySize, charSize);
    SDLNet_TCP_Recv(this->socket, serverPublicKey, (int) publicKeySize);
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    CryptoCryptDetails* cryptDetails = SDL_malloc(sizeof *cryptDetails);
    cryptDetails->blockSize = PADDING_BLOCK_SIZE;
    cryptDetails->unpaddedSize = MESSAGE_SIZE;

    byte* clientPublicKey = cryptoInit(serverPublicKey, cryptDetails);
    if (!clientPublicKey) return;

    SDLNet_TCP_Send(this->socket, clientPublicKey, (int) publicKeySize);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;

    Message* msg = SDL_malloc(sizeof *msg); // TODO: test only
    msg->flag = FLAG_FINISH;
    msg->timestamp = 0;
    msg->size = MESSAGE_BODY_SIZE;
    msg->index = 0;
    msg->count = 1;

    const char test[] = "Test connection"; // TODO: test only
    const unsigned testLen = sizeof test / sizeof *test;
    SDL_memset(msg->body, 0, MESSAGE_BODY_SIZE);
    SDL_memcpy(msg->body, test, testLen);

    byte* bytes = packMessage(msg); // TODO: test only
    SDLNet_TCP_Send(this->socket, bytes, (int) MESSAGE_SIZE);
    SDL_free(bytes);
}

bool netInit() { // TODO: add compression
    unsigned long byteOrderChecker = 0x0123456789abcdefl;
    assert(*((byte*) &byteOrderChecker) == 0xef); // checks whether the app is running on a x64 littleEndian architecture so the byte order won't mess up data marshalling

    this = SDL_malloc(sizeof *this);
    this->socket = NULL;
    this->socketSet = NULL;
    assert(!SDLNet_Init());

    IPaddress address;
    assert(!SDLNet_ResolveHost(&address, NET_HOST, PORT));

    this->socket = SDLNet_TCP_OpenClient(&address);
    if (!this->socket) {
        netClean();
        return false;
    }

    this->socketSet = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(this->socketSet, this->socket);

    initiateSecuredConnection();
    bool result = this->state == STATE_READY;

    if (!result) netClean();
    return result;
}

static bool isDataAvailable() {
    return SDLNet_CheckSockets(this->socketSet, 0) == 1
        && SDLNet_SocketReady(this->socket) != 0;
}

static Message* unpackMessage(byte* buffer) {
    Message* msg = SDL_malloc(sizeof *msg);
    const unsigned intSize = sizeof(int), longSize = sizeof(long);

    SDL_memcpy(&(msg->flag), buffer, intSize);
    SDL_memcpy(&(msg->timestamp), buffer + intSize, longSize);
    SDL_memcpy(&(msg->size), buffer + intSize + longSize, intSize);
    SDL_memcpy(&(msg->index), buffer + intSize * 2 + longSize, intSize);
    SDL_memcpy(&(msg->count), buffer + intSize * 3 + longSize, intSize);
    SDL_memcpy(&(msg->body), buffer + MESSAGE_HEAD_SIZE, MESSAGE_BODY_SIZE);

    SDL_free(buffer);
    return msg;
}

static byte* packMessage(Message* msg) {
    byte* buffer = SDL_calloc(MESSAGE_SIZE, sizeof(char));
    const unsigned intSize = sizeof(int), longSize = sizeof(long);

    SDL_memcpy(buffer, &(msg->flag), intSize);
    SDL_memcpy(buffer + intSize, &(msg->timestamp), longSize);
    SDL_memcpy(buffer + intSize + longSize, &(msg->size), intSize);
    SDL_memcpy(buffer + intSize * 2 + longSize, &(msg->index), intSize);
    SDL_memcpy(buffer + intSize * 3 + longSize, &(msg->count), intSize);
    SDL_memcpy(buffer + MESSAGE_HEAD_SIZE, &(msg->body), MESSAGE_BODY_SIZE);

    SDL_free(msg);
    return buffer;
}

void netListen() {
    Message* msg = NULL;

    if (isDataAvailable()) {
        byte* buffer = SDL_calloc(receiveBufferSize(), sizeof(char));

        SDLNet_TCP_Recv(this->socket, buffer, (int) receiveBufferSize());
        msg = unpackMessage(buffer);
    }

    switch (this->state) {
        case STATE_READY: break; // TODO
    }

    SDL_free(msg);
}

void netSend(byte* message, unsigned size) {
    Message* msg = SDL_malloc(sizeof *msg);
    msg->flag = 0;
    msg->timestamp = 0;
    msg->size = 0;
    msg->index = 0;
    msg->count = 0;

    SDL_memcpy(&(msg->body), message, size);
    SDL_free(message);

    byte* buffer = packMessage(msg);
    byte* encryptedBuffer = cryptoEncrypt(buffer);
    if (!encryptedBuffer) return;

    SDLNet_TCP_Send(this->socket, buffer, (int) receiveBufferSize());

    SDL_free(encryptedBuffer);
}

void netClean() {
    cryptoClean();
    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();
    SDL_free(this);
}
