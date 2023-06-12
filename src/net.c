
#include <sdl_net/SDL_net.h>
#include <assert.h>
#include "crypto.h"
#include "net.h"

#define STATE(x, y) static const int STATE_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(SERVER_PUBLIC_KEY_RECEIVED, 1)
STATE(CLIENT_PUBLIC_KEY_SENT, 2)
STATE(READY, STATE_CLIENT_PUBLIC_KEY_SENT) // TODO: check client signature on server and check server signature on client

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS( // TODO: check client's authentication by token
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    int state;
    unsigned receiveBufferSize;
    Function onMessageReceived;
)
#pragma clang diagnostic pop

static const int PORT = 8080;
static const unsigned MESSAGE_HEAD_SIZE = sizeof(int) * 6 + sizeof(long); // 32
static const unsigned MESSAGE_BODY_SIZE = 1 << 10; // 1024
static const unsigned MESSAGE_SIZE = MESSAGE_HEAD_SIZE + MESSAGE_BODY_SIZE; // 1056
static const unsigned PADDING_BLOCK_SIZE = 16;
static const int FLAG_UNAUTHENTICATED = 0x7ffffffe;
static const int FLAG_FINISH = 0x7fffffff;
static const unsigned INT_SIZE = sizeof(int);
static const unsigned LONG_SIZE = sizeof(long);

typedef struct {
    // begin head
    int flag; // short service description of message
    unsigned long timestamp; // message created at
    unsigned size; // actual size of th payload
    unsigned index; // message part index if the whole message cannot fit in boy
    unsigned count; // total count of message parts
    unsigned from; // id of the sender
    unsigned to; // id of the receiver
    // end head

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-folding-constant" // If this causes compile troubles declare this constant as a macro
    byte body[MESSAGE_BODY_SIZE]; // payload
#pragma clang diagnostic pop

} Message;

static byte* packMessage(Message* msg); // TODO: test only
static Message* unpackMessage(byte* buffer);

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
    this->receiveBufferSize = cryptoEncryptedSize();

    SDLNet_TCP_Send(this->socket, clientPublicKey, (int) publicKeySize);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;

    Message* msg = SDL_malloc(sizeof *msg); // TODO: test only
    msg->flag = FLAG_FINISH;
    msg->timestamp = 0;
    msg->size = MESSAGE_BODY_SIZE;
    msg->index = 0;
    msg->count = 1;
    msg->from = 255;
    msg->to = 255;

    const char test[] = "Test connection"; // TODO: test only
    const unsigned testLen = sizeof test / sizeof *test;
    SDL_memset(msg->body, 0, MESSAGE_BODY_SIZE);
    SDL_memcpy(msg->body, test, testLen);

    byte* bytes = packMessage(msg); // TODO: test only
    byte* encrypted = cryptoEncrypt(bytes);
    SDLNet_TCP_Send(this->socket, encrypted, (int) this->receiveBufferSize);
    SDL_free(encrypted);
}

bool netInit(Function onMessageReceived) { // TODO: add compression
    unsigned long byteOrderChecker = 0x0123456789abcdefl;
    assert(*((byte*) &byteOrderChecker) == 0xef); // checks whether the app is running on a x64 littleEndian architecture so the byte order won't mess up data marshalling

    this = SDL_malloc(sizeof *this);
    this->socket = NULL;
    this->socketSet = NULL;
    this->onMessageReceived = onMessageReceived;
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"
unsigned netMessageSize() { return MESSAGE_BODY_SIZE; } // reveals only one constant to the users of this api
#pragma clang diagnostic pop

static bool isDataAvailable() {
    return SDLNet_CheckSockets(this->socketSet, 0) == 1
        && SDLNet_SocketReady(this->socket) != 0;
}

static Message* unpackMessage(byte* buffer) {
    Message* msg = SDL_malloc(sizeof *msg);

    SDL_memcpy(&(msg->flag), buffer, INT_SIZE);
    SDL_memcpy(&(msg->timestamp), buffer + INT_SIZE, LONG_SIZE);
    SDL_memcpy(&(msg->size), buffer + INT_SIZE + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->index), buffer + INT_SIZE * 2 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->count), buffer + INT_SIZE * 3 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->from), buffer + INT_SIZE * 4 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->to), buffer + INT_SIZE * 5 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->body), buffer + MESSAGE_HEAD_SIZE, MESSAGE_BODY_SIZE);

    SDL_free(buffer);
    return msg;
}

static byte* packMessage(Message* msg) {
    byte* buffer = SDL_calloc(MESSAGE_SIZE, sizeof(char));

    SDL_memcpy(buffer, &(msg->flag), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE, &(msg->timestamp), LONG_SIZE);
    SDL_memcpy(buffer + INT_SIZE + LONG_SIZE, &(msg->size), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 2 + LONG_SIZE, &(msg->index), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 3 + LONG_SIZE, &(msg->count), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 4 + LONG_SIZE, &(msg->from), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 5 + LONG_SIZE, &(msg->to), INT_SIZE);
    SDL_memcpy(buffer + MESSAGE_HEAD_SIZE, &(msg->body), MESSAGE_BODY_SIZE);

    SDL_free(msg);
    return buffer;
}

void netListen() {
    Message* msg = NULL;

    if (isDataAvailable()) {
        byte* buffer = SDL_calloc(this->receiveBufferSize, sizeof(char)); // TODO: move buffer outside the function to initialize it only once and not on each listen event

        if (SDLNet_TCP_Recv(this->socket, buffer, (int) this->receiveBufferSize) == (int) this->receiveBufferSize) {
            byte* decrypted = cryptoDecrypt(buffer);
            msg = unpackMessage(decrypted);
        }
    }

    switch (this->state) {
        case STATE_READY:
            if (msg) SDL_Log("%u %lu %u %u %u %u %u\n", msg->flag, msg->timestamp, msg->size, msg->index, msg->count, msg->from, msg->to); // TODO: test only
            if (msg) this->onMessageReceived(msg->body);
            break;
    }

    SDL_free(msg);
}

void netSend(byte* message, unsigned size) {
    assert(size <= MESSAGE_BODY_SIZE);

    Message* msg = SDL_malloc(sizeof *msg);
    msg->flag = 0;
    msg->timestamp = 0;
    msg->size = size;
    msg->index = 0;
    msg->count = 0;
    msg->from = 255;
    msg->to = 255;

    SDL_memset(msg->body, 0, MESSAGE_BODY_SIZE);
    SDL_memcpy(&(msg->body), message, size);
    SDL_free(message);

    byte* buffer = packMessage(msg);
    byte* encryptedBuffer = cryptoEncrypt(buffer);
    if (!encryptedBuffer) return;

    SDLNet_TCP_Send(this->socket, encryptedBuffer, (int) this->receiveBufferSize);
    SDL_free(encryptedBuffer);
}

void netClean() {
    cryptoClean();
    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();
    SDL_free(this);
}
