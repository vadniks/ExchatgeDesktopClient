
#include <sdl_net/SDL_net.h>
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
static const int MESSAGE_HEAD_SIZE = sizeof(int) * 4 + sizeof(long);
static const int MESSAGE_BODY_SIZE = 1 << 10; // 1024
static const int MESSAGE_SIZE = MESSAGE_HEAD_SIZE + MESSAGE_BODY_SIZE;
static const int FLAG_UNAUTHENTICATED = 0x7ffffffe;
static const int FLAG_FINISH = 0x7fffffff;
static const int PADDING_BLOCK_SIZE = 16;

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

static void initiateSecuredConnection() {
    const unsigned publicKeySize = crPublicKeySize(),
        charSize = sizeof(char);

    byte* serverPublicKey = SDL_calloc(publicKeySize, charSize);
    SDLNet_TCP_Recv(this->socket, serverPublicKey, (int) publicKeySize);
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    CrCryptDetails* cryptDetails = SDL_malloc(sizeof *cryptDetails);
    cryptDetails->blockSize = 16;
    cryptDetails->unpaddedSize = MESSAGE_SIZE;

    byte* clientPublicKey = crInit(serverPublicKey, cryptDetails);
    if (!clientPublicKey) return;

    SDLNet_TCP_Send(this->socket, clientPublicKey, (int) publicKeySize);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;
}

#include <stdio.h> // TODO: test only
static Message* unpackMessage(byte* buffer);
static byte* packMessage(Message* msg);

bool ntInit() { // TODO: add compression
    this = SDL_malloc(sizeof *this);
    this->socket = NULL;
    this->socketSet = NULL;
    SDLNet_Init();

    Message* msg = SDL_malloc(sizeof *msg); // TODO: test only
    msg->flag = 1234567890;
    msg->timestamp = 0;
    msg->size = MESSAGE_BODY_SIZE;
    msg->index = 0;
    msg->count = 1;

    const char* test = "Test"; // TODO: test only
    SDL_memcpy(msg->body, test, 4);
    for (unsigned i = 0; i < MESSAGE_BODY_SIZE; printf("%u ", msg->body[i++])); // TODO: and how am I supposed to port all these thing to Go?
    printf("\n");

    byte* packed = packMessage(msg); // TODO: test only
    for (unsigned i = 0; i < MESSAGE_BODY_SIZE; printf("%u ", packed[i++]));
    printf("\n");

    CrCryptDetails* cryptDetails = SDL_malloc(sizeof *cryptDetails); // TODO: test only
    cryptDetails->blockSize = PADDING_BLOCK_SIZE;
    cryptDetails->unpaddedSize = MESSAGE_SIZE;
    crInit(NULL, cryptDetails);

    byte* encrypted = crEncrypt(packed); // TODO: test only
    for (unsigned i = 0; i < crEncryptedSize(); printf("%u ", encrypted[i++]));
    printf("\n");

    byte* decrypted = crDecrypt(encrypted); // TODO: test only

    Message* unpacked = unpackMessage(decrypted); // TODO: test only
    printf("%d %ld %d %d %d\n", unpacked->flag, unpacked->timestamp, unpacked->size, unpacked->index, unpacked->count);
    for (unsigned i = 0; i < MESSAGE_BODY_SIZE; printf("%u ", unpacked->body[i++]));
    printf("\n");
    printf("%s\n", unpacked->body);
    SDL_free(unpacked); // TODO: works fine!

    IPaddress address;
    SDLNet_ResolveHost(&address, NET_HOST, PORT);

    this->socket = SDLNet_TCP_OpenClient(&address);
    if (!this->socket) {
        ntClean();
        return false;
    }

    this->socketSet = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(this->socketSet, this->socket);

    initiateSecuredConnection();
    bool result = this->state == STATE_READY;

    if (!result) ntClean();
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

static bool test = false; // TODO: test only
#define NET_RECEIVE_BUFFER_SIZE 1
void ntListen() {
    Message* msg = NULL;

    if (isDataAvailable()) {
        byte* buffer = SDL_calloc(NET_RECEIVE_BUFFER_SIZE, sizeof(char));

        SDLNet_TCP_Recv(this->socket, buffer, NET_RECEIVE_BUFFER_SIZE);
        msg = unpackMessage(buffer);
    }

    switch (this->state) {
        case STATE_READY:
            if (test) break;
            test = true;

            const char* text = "Hello World!";
            byte* testMsg = SDL_malloc(12 * sizeof(char));
            SDL_memcpy(testMsg, text, 12);
            ntSend(testMsg, 12); // TODO: test only

            SDL_Log("hello world sent");

            break; // TODO
    }

    SDL_free(msg);
}

void ntSend(byte* bytes, unsigned size) {
    Message* msg = SDL_malloc(sizeof *msg);
    msg->flag = 0;
    msg->timestamp = 0;
    msg->size = 0;
    msg->index = 0;
    msg->count = 0;

    SDL_memcpy(&(msg->body), bytes, size);
    SDL_free(bytes);

    byte* buffer = packMessage(msg);
    byte* encryptedBuffer = crEncrypt(buffer);
    if (!encryptedBuffer) return;

    SDLNet_TCP_Send(this->socket, buffer, NET_RECEIVE_BUFFER_SIZE);

    SDL_free(encryptedBuffer);
}

void ntClean() {
    crClean();
    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();
    SDL_free(this);
}
