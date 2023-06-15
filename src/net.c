
// TODO: check client's authentication by token
// TODO: check client signature on server and check server signature on client
// TODO: add compression

#include <sdl_net/SDL_net.h>
#include <assert.h>
#include "crypto.h"
#include "net.h"

#define STATE(x, y) static const int STATE_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(SERVER_PUBLIC_KEY_RECEIVED, 1)
STATE(CLIENT_PUBLIC_KEY_SENT, 2)
STATE(READY, STATE_CLIENT_PUBLIC_KEY_SENT)

static const int PORT = 8080;
static const unsigned MESSAGE_HEAD_SIZE = sizeof(int) * 6 + sizeof(long); // 32
static const unsigned MESSAGE_BODY_SIZE = 1 << 10; // 1024
static const unsigned MESSAGE_SIZE = MESSAGE_HEAD_SIZE + MESSAGE_BODY_SIZE; // 1056
static const unsigned PADDING_BLOCK_SIZE = 16;
static const int FLAG_UNAUTHENTICATED = 0x7ffffffe;
static const int FLAG_FINISH = 0x7fffffff;
static const unsigned INT_SIZE = sizeof(int);
static const unsigned LONG_SIZE = sizeof(long);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    int state;
    unsigned encryptedMessageSize;
    MessageReceivedCallback onMessageReceived;
    Crypto* connectionCrypto; // client-server level encryption - different for each connection
//    Crypto* conversationCrypto; // TODO: conversation level encryption - extra encryption layer - different for each conversation but permanent for all participants of a conversation
    byte* messageBuffer;
)
#pragma clang diagnostic pop

#pragma pack(true)

typedef struct {
    int flag; // short service description of message
    unsigned long timestamp; // message created at
    unsigned size; // actual size of th payload
    unsigned index; // message part index if the whole message cannot fit in boy
    unsigned count; // total count of message parts
    unsigned from; // id of the sender
    unsigned to; // id of the receiver
} MessageHead;

typedef struct {
    MessageHead;
    byte body[MESSAGE_BODY_SIZE]; // payload
} Message;

staticAssert(sizeof(Message) == 1056);

static byte* packMessage(const Message* msg); // TODO: test only
static Message* unpackMessage(const byte* buffer);

static void initiateSecuredConnection(void) {
    byte serverPublicKey[CRYPTO_KEY_SIZE];
    SDLNet_TCP_Recv(this->socket, serverPublicKey, (int) CRYPTO_KEY_SIZE);
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    this->connectionCrypto = cryptoInit();
    if (!(this->connectionCrypto)) return;

    cryptoSetServerPublicKey(this->connectionCrypto, serverPublicKey);
    if (cryptoExchangeKeys(this->connectionCrypto)) return;
    this->encryptedMessageSize = cryptoEncryptedSize(MESSAGE_SIZE);

    SDLNet_TCP_Send(this->socket, cryptoClientPublicKey(this->connectionCrypto), (int) CRYPTO_KEY_SIZE);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;

    Message message = { // TODO: test only
        {
            FLAG_FINISH,
            0,
            MESSAGE_BODY_SIZE,
            0,
            1,
            255,
            255
        },
        { 0 }
    };

    const char test[] = "Test connection"; // TODO: test only
    const unsigned testLen = sizeof test / sizeof *test;
    SDL_memset(message.body, 0, MESSAGE_BODY_SIZE);
    SDL_memcpy(message.body, test, testLen);

    byte* bytes = packMessage(&message); // TODO: test only
    byte* encrypted = cryptoEncrypt(this->connectionCrypto, bytes, MESSAGE_SIZE); // TODO: test only
    SDL_free(bytes);

    SDLNet_TCP_Send(this->socket, encrypted, (int) this->encryptedMessageSize); // TODO: test only
    SDL_free(encrypted);
}

bool netInit(MessageReceivedCallback onMessageReceived) {
    assert(!this && onMessageReceived);

    unsigned long byteOrderChecker = 0x0123456789abcdefl;
    assert(*((byte*) &byteOrderChecker) == 0xef); // checks whether the app is running on a x64 littleEndian architecture so the byte order won't mess up data marshalling

    this = SDL_malloc(sizeof *this);
    this->socket = NULL;
    this->socketSet = NULL;
    this->onMessageReceived = onMessageReceived;
    this->connectionCrypto = NULL;
    this->messageBuffer = NULL;
    assert(!SDLNet_Init());

    IPaddress address;
    assert(!SDLNet_ResolveHost(&address, NET_HOST, PORT));

    this->socket = SDLNet_TCP_OpenClient(&address);
    if (!this->socket) {
        netClean();
        return false;
    }

    this->socketSet = SDLNet_AllocSocketSet(1);
    assert(this->socketSet);
    assert(SDLNet_TCP_AddSocket(this->socketSet, this->socket) == 1);

    initiateSecuredConnection();
    this->messageBuffer = SDL_malloc(this->encryptedMessageSize);

    if (this->state != STATE_READY) {
        netClean();
        return false;
    } else
        return true;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"
unsigned netMessageSize(void) { return MESSAGE_BODY_SIZE; } // reveals only one constant to the users of this api
#pragma clang diagnostic pop

static Message* unpackMessage(const byte* buffer) {
    Message* msg = SDL_malloc(sizeof *msg);

    SDL_memcpy(&(msg->flag), buffer, INT_SIZE);
    SDL_memcpy(&(msg->timestamp), buffer + INT_SIZE, LONG_SIZE);
    SDL_memcpy(&(msg->size), buffer + INT_SIZE + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->index), buffer + INT_SIZE * 2 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->count), buffer + INT_SIZE * 3 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->from), buffer + INT_SIZE * 4 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->to), buffer + INT_SIZE * 5 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->body), buffer + MESSAGE_HEAD_SIZE, MESSAGE_BODY_SIZE);

    return msg;
}

static byte* packMessage(const Message* msg) {
    byte* buffer = SDL_calloc(MESSAGE_SIZE, sizeof(char));

    SDL_memcpy(buffer, &(msg->flag), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE, &(msg->timestamp), LONG_SIZE);
    SDL_memcpy(buffer + INT_SIZE + LONG_SIZE, &(msg->size), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 2 + LONG_SIZE, &(msg->index), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 3 + LONG_SIZE, &(msg->count), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 4 + LONG_SIZE, &(msg->from), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 5 + LONG_SIZE, &(msg->to), INT_SIZE);
    SDL_memcpy(buffer + MESSAGE_HEAD_SIZE, &(msg->body), MESSAGE_BODY_SIZE);

    return buffer;
}

void netListen(void) {
    Message* msg = NULL;

    if (SDLNet_CheckSockets(this->socketSet, 0) == 1 && SDLNet_SocketReady(this->socket) != 0) {

        if (SDLNet_TCP_Recv(this->socket, this->messageBuffer, (int) this->encryptedMessageSize) == (int) this->encryptedMessageSize) {
            byte* decrypted = cryptoDecrypt(this->connectionCrypto, this->messageBuffer, MESSAGE_SIZE);
            assert(decrypted);

            msg = unpackMessage(decrypted);
            SDL_free(decrypted);
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

void netSend(const byte* bytes, unsigned size) {
    assert(bytes && size > 0 && size <= MESSAGE_BODY_SIZE);

    Message message = {
        {
            0,
            0,
            size,
            0,
            0,
            255,
            255
        },
        { 0 }
    };
    SDL_memcpy(&(message.body), bytes, size);

    byte* packedMessage = packMessage(&message);
    byte* encryptedMessage = cryptoEncrypt(this->connectionCrypto, packedMessage, MESSAGE_SIZE);
    SDL_free(packedMessage);
    if (!encryptedMessage) return;

    SDLNet_TCP_Send(this->socket, encryptedMessage, (int) this->encryptedMessageSize);
    SDL_free(encryptedMessage);
}

void netClean(void) {
    assert(this);

    SDL_free(this->messageBuffer);
    if (this->connectionCrypto) cryptoDestroy(this->connectionCrypto);

    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();

    SDL_free(this);
    this = NULL;
}
