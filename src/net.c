
// TODO: check client's authentication by token
// TODO: check client signature on server and check server signature on client
// TODO: add compression

#include <sdl_net/SDL_net.h>
#include <assert.h>
#include <time.h>
#include "crypto.h"
#include "net.h"

staticAssert(sizeof(char) == 1 && sizeof(int) == 4 && sizeof(long) == 8 && sizeof(void*) == 8);

#define STATE(x, y)  STATIC_CONST_UNSIGNED STATE_ ## x = y;
#define FLAG(x, y) STATIC_CONST_INT FLAG_ ## x = y;

STATE(DISCONNECTED, 0)
STATE(SERVER_PUBLIC_KEY_RECEIVED, 1)
STATE(CLIENT_PUBLIC_KEY_SENT, 2)
STATE(SECURE_CONNECTION_ESTABLISHED, STATE_CLIENT_PUBLIC_KEY_SENT)
STATE(AUTHENTICATED, 3)
STATE(FINISHED, 4)
STATE(FINISHED_WITH_ERROR, 5)

static const char* HOST = "127.0.0.1";
STATIC_CONST_UNSIGNED PORT = 8080;

STATIC_CONST_UNSIGNED INT_SIZE = sizeof(int);
STATIC_CONST_UNSIGNED LONG_SIZE = sizeof(long);

STATIC_CONST_UNSIGNED MESSAGE_SIZE = 1 << 10; // 1024
STATIC_CONST_UNSIGNED TOKEN_TRAILING_SIZE = 16;
STATIC_CONST_UNSIGNED TOKEN_UNSIGNED_VALUE_SIZE = 2 * INT_SIZE; // 8
STATIC_CONST_UNSIGNED TOKEN_SIZE = TOKEN_UNSIGNED_VALUE_SIZE + 40 + TOKEN_TRAILING_SIZE; // 64
STATIC_CONST_UNSIGNED MESSAGE_HEAD_SIZE = INT_SIZE * 6 + LONG_SIZE + TOKEN_SIZE; // 96
const unsigned NET_MESSAGE_BODY_SIZE = MESSAGE_SIZE - MESSAGE_HEAD_SIZE; // 928

FLAG(PROCEED, 0x00000000)
FLAG(FINISH, 0x00000001)
FLAG(FINISH_WITH_ERROR, 0x00000002)
FLAG(FINISH_TO_RECONNECT, 0x00000003)
FLAG(LOG_IN, 0x00000004)
FLAG(LOGGED_IN, 0x00000005)
FLAG(REGISTER, 0x00000006)
FLAG(REGISTERED, 0x00000007)
FLAG(SUCCESS, 0x00000008)
FLAG(ERROR, 0x00000009)
FLAG(UNAUTHENTICATED, 0x0000000a)
FLAG(ACCESS_DENIED, 0x0000000b)
FLAG(FETCH_ALL, 0x0000000c)
FLAG(SHUTDOWN, 0x7fffffff)

STATIC_CONST_UNSIGNED TO_ANONYMOUS = 0x7fffffff;
STATIC_CONST_UNSIGNED TO_SERVER = 0x7ffffffe;

const unsigned NET_USERNAME_SIZE = 16;
const unsigned NET_UNHASHED_PASSWORD_SIZE = 16;

STATIC_CONST_UNSIGNED FROM_ANONYMOUS = 0xffffffff;
STATIC_CONST_UNSIGNED FROM_SERVER = 0x7fffffff;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    unsigned state;
    unsigned encryptedMessageSize; // constant
    NetMessageReceivedCallback onMessageReceived;
    Crypto* connectionCrypto; // client-server level encryption - different for each connection
//    Crypto* conversationCrypto; // TODO: conversation level encryption - extra encryption layer - different for each conversation but permanent for all participants of a conversation
    byte* messageBuffer;
    byte tokenAnonymous[TOKEN_SIZE]; // constant
    byte tokenServerUnsignedValue[TOKEN_UNSIGNED_VALUE_SIZE]; // constant, unencrypted but clients don't know how token is generated
    byte token[TOKEN_SIZE];
    unsigned userId;
    NetNotifierCallback onLogInResult;
    NetServiceCallback onErrorReceived;
    NetNotifierCallback onRegisterResult;
    NetCallback onDisconnected;
    NetCurrentTimeMillisGetter currentTimeMillisGetter;
    int lastSentFlag;
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
    byte token[TOKEN_SIZE];
} MessageHead;

typedef struct {
    MessageHead;
    byte body[NET_MESSAGE_BODY_SIZE]; // payload
} Message;

staticAssert(sizeof(MessageHead) == 96 && sizeof(Message) == 1024 && sizeof(Message) - sizeof(MessageHead) == 928);

static void initiateSecuredConnection(void) {
    const unsigned signedPublicKeySize = CRYPTO_SIGNATURE_SIZE + CRYPTO_KEY_SIZE;
    byte serverSignedPublicKey[signedPublicKeySize];

    SDLNet_TCP_Recv(this->socket, serverSignedPublicKey, (int) signedPublicKeySize);
    assert(cryptoCheckServerSignedBytes(serverSignedPublicKey, serverSignedPublicKey + CRYPTO_SIGNATURE_SIZE, CRYPTO_KEY_SIZE));
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    this->connectionCrypto = cryptoInit();
    assert(this->connectionCrypto);

    if (!cryptoExchangeKeys(this->connectionCrypto, serverSignedPublicKey + CRYPTO_SIGNATURE_SIZE)) return;
    this->encryptedMessageSize = cryptoEncryptedSize(MESSAGE_SIZE);

    SDLNet_TCP_Send(this->socket, cryptoClientPublicKey(this->connectionCrypto), (int) CRYPTO_KEY_SIZE);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;
}

bool netInit(
    NetMessageReceivedCallback onMessageReceived,
    NetNotifierCallback onLogInResult,
    NetServiceCallback onErrorReceived,
    NetNotifierCallback onRegisterResult,
    NetCallback onDisconnected,
    NetCurrentTimeMillisGetter currentTimeMillisGetter
) {
    assert(!this && onMessageReceived && onLogInResult && onErrorReceived && onDisconnected);

    unsigned long byteOrderChecker = 0x0123456789abcdeful; // just for notice - u & l at the end stand for unsigned long, they're not hexits (digit for hex analogue), leading 0 & x defines hex numbering system
    assert(*((byte*) &byteOrderChecker) == 0xef); // checks whether the app is running on a x64 littleEndian architecture so the byte order won't mess up data marshalling

    this = SDL_malloc(sizeof *this);
    this->socket = NULL;
    this->socketSet = NULL;
    this->onMessageReceived = onMessageReceived;
    this->connectionCrypto = NULL;
    this->messageBuffer = NULL;
    SDL_memset(this->tokenAnonymous, 0, TOKEN_SIZE);
    SDL_memset(this->tokenServerUnsignedValue, (1 << 8) - 1, TOKEN_UNSIGNED_VALUE_SIZE);
    SDL_memcpy(this->token, this->tokenAnonymous, TOKEN_SIZE); // until user is authenticated hist token is anonymous
    this->userId = FROM_ANONYMOUS;
    this->onLogInResult = onLogInResult;
    this->onErrorReceived = onErrorReceived;
    this->onRegisterResult = onRegisterResult;
    this->onDisconnected = onDisconnected;
    this->currentTimeMillisGetter = currentTimeMillisGetter;

    assert(!SDLNet_Init());

    IPaddress address;
    assert(!SDLNet_ResolveHost(&address, HOST, PORT));

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

    if (this->state != STATE_SECURE_CONNECTION_ESTABLISHED) {
        netClean();
        return false;
    } else
        return true;
}

static bool checkServerToken(const byte* token)
{ return cryptoCheckServerSignedBytes(token, this->tokenServerUnsignedValue, TOKEN_UNSIGNED_VALUE_SIZE); }

static byte* makeCredentials(const char* username, const char* password) {
    byte* credentials = SDL_malloc(NET_USERNAME_SIZE + NET_UNHASHED_PASSWORD_SIZE);
    SDL_memcpy(credentials, username, NET_USERNAME_SIZE);
    SDL_memcpy(&(credentials[NET_USERNAME_SIZE]), password, NET_UNHASHED_PASSWORD_SIZE);
    return credentials;
}

void netLogIn(const char* username, const char* password) { // TODO: store both username & password encrypted inside a client
    assert(this);
    byte* credentials = makeCredentials(username, password);
    netSend(FLAG_LOG_IN, credentials, NET_USERNAME_SIZE + NET_UNHASHED_PASSWORD_SIZE, TO_SERVER);
    SDL_free(credentials);
}

void netRegister(const char* username, const char* password) {
    assert(this);
    byte* credentials = makeCredentials(username, password);
    netSend(FLAG_REGISTER, credentials, NET_USERNAME_SIZE + NET_UNHASHED_PASSWORD_SIZE, TO_SERVER);
    SDL_free(credentials);
}

static Message* unpackMessage(const byte* buffer) {
    Message* msg = SDL_malloc(sizeof *msg);

    SDL_memcpy(&(msg->flag), buffer, INT_SIZE);
    SDL_memcpy(&(msg->timestamp), buffer + INT_SIZE, LONG_SIZE);
    SDL_memcpy(&(msg->size), buffer + INT_SIZE + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->index), buffer + INT_SIZE * 2 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->count), buffer + INT_SIZE * 3 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->from), buffer + INT_SIZE * 4 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->to), buffer + INT_SIZE * 5 + LONG_SIZE, INT_SIZE);
    SDL_memcpy(&(msg->token), buffer + INT_SIZE * 6 + LONG_SIZE, TOKEN_SIZE);
    SDL_memcpy(&(msg->body), buffer + MESSAGE_HEAD_SIZE, NET_MESSAGE_BODY_SIZE);

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
    SDL_memcpy(buffer + INT_SIZE * 6 + LONG_SIZE, &(msg->token), TOKEN_SIZE);
    SDL_memcpy(buffer + MESSAGE_HEAD_SIZE, &(msg->body), NET_MESSAGE_BODY_SIZE);

    return buffer;
}

static void processErrors(const Message* message) {
    switch (this->lastSentFlag) {
        case FLAG_LOG_IN:
            this->state = STATE_FINISHED_WITH_ERROR;
            (*(this->onLogInResult))(false);
            break;
        case FLAG_REGISTER:
            (*(this->onRegisterResult))(false);
            break;
        default:
            (*(this->onErrorReceived))(message->flag);
            break;
    }
}

static void processServerActions(const Message* message) {
    switch (message->flag) {
        case FLAG_LOGGED_IN:
            this->state = STATE_AUTHENTICATED;
            this->userId = message->to;
            SDL_memcpy(this->token, message->body, TOKEN_SIZE);

            (*(this->onLogInResult))(true);
            break;
        case FLAG_REGISTERED:
            (*(this->onRegisterResult))(true);
            break;
    }
}

static void processMessage(const Message* message) {
    if (message->from == FROM_SERVER) {
        assert(checkServerToken(message->token));

        const int flag = message->flag;
        if (flag == FLAG_UNAUTHENTICATED || flag == FLAG_ERROR || flag == FLAG_ACCESS_DENIED) processErrors(message);
    }

    switch (this->state) {
        case STATE_SECURE_CONNECTION_ESTABLISHED:
            processServerActions(message);
            break;
        case STATE_AUTHENTICATED:
            (*(this->onMessageReceived))(message->body);
            break;
    }
}

static void onDisconnected(void) {
    (*(this->onDisconnected))();
    netClean();
}

void netListen(void) {
    assert(this);
    Message* message = NULL;

    if (SDLNet_CheckSockets(this->socketSet, 0) == 1 && SDLNet_SocketReady(this->socket) != 0) {

        if (SDLNet_TCP_Recv(this->socket, this->messageBuffer, (int) this->encryptedMessageSize) == (int) this->encryptedMessageSize) {
            byte* decrypted = cryptoDecrypt(this->connectionCrypto, this->messageBuffer, this->encryptedMessageSize);
            assert(decrypted);

            message = unpackMessage(decrypted);
            SDL_free(decrypted);
        } else { // TODO: send finish message to server on normal cleanup (when client normally shutdowns)
            onDisconnected();
            return;
        }
    }

    if (message) processMessage(message);
    SDL_free(message);
}

void netSend(int flag, const byte* body, unsigned size, unsigned xTo) {
    assert(this && size > 0 && size <= NET_MESSAGE_BODY_SIZE);

    Message message = {
        {
            flag,
            (*(this->currentTimeMillisGetter))(),
            size,
            0,
            1,
            this->userId,
            xTo,
            { 0 }
        },
        { 0 }
    };
    SDL_memcpy(&(message.body), body, size);
    SDL_memcpy(&(message.token), this->token, TOKEN_SIZE);

    byte* packedMessage = packMessage(&message);
    byte* encryptedMessage = cryptoEncrypt(this->connectionCrypto, packedMessage, MESSAGE_SIZE);
    SDL_free(packedMessage);
    if (!encryptedMessage) return;

    this->lastSentFlag = flag;
    SDLNet_TCP_Send(this->socket, encryptedMessage, (int) this->encryptedMessageSize);
    SDL_free(encryptedMessage);
}

void netShutdownServer(void) {
    assert(this);

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    netSend(FLAG_SHUTDOWN, body, NET_MESSAGE_BODY_SIZE, TO_SERVER);
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
