/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <SDL_net.h>
#include <assert.h>
#include <endian.h>
#include "utils/rwMutex.h"
#include "net.h"

staticAssert(sizeof(char) == 1 && sizeof(int) == 4 && sizeof(long) == 8 && sizeof(void*) == 8);

#if BYTE_ORDER != LITTLE_ENDIAN
#   error "Target is not little endian"
#endif

typedef enum : unsigned {
    STATE_SERVER_PUBLIC_KEY_RECEIVED = 1,
    STATE_CLIENT_PUBLIC_KEY_SENT = 2,
    STATE_SERVER_CODER_HEADER_RECEIVED = 3,
    STATE_CLIENT_CODER_HEADER_SENT = 4,
    STATE_SECURE_CONNECTION_ESTABLISHED = STATE_CLIENT_CODER_HEADER_SENT,
    STATE_AUTHENTICATED = 5,
    STATE_FINISHED_WITH_ERROR = 7
} States;

STATIC_CONST_UNSIGNED INT_SIZE = sizeof(int);
STATIC_CONST_UNSIGNED LONG_SIZE = sizeof(long);

STATIC_CONST_UNSIGNED MESSAGE_SIZE = 1 << 10; // 1024
STATIC_CONST_UNSIGNED TOKEN_TRAILING_SIZE = 16;
STATIC_CONST_UNSIGNED TOKEN_UNSIGNED_VALUE_SIZE = 2 * INT_SIZE; // 8
STATIC_CONST_UNSIGNED TOKEN_SIZE = TOKEN_UNSIGNED_VALUE_SIZE + 40 + TOKEN_TRAILING_SIZE; // 64
STATIC_CONST_UNSIGNED MESSAGE_HEAD_SIZE = INT_SIZE * 6 + LONG_SIZE + TOKEN_SIZE; // 96
const unsigned NET_MESSAGE_BODY_SIZE = MESSAGE_SIZE - MESSAGE_HEAD_SIZE; // 928

STATIC_CONST_UNSIGNED long TIMEOUT = 5000; // in milliseconds

typedef enum : int {
    FLAG_PROCEED = 0x00000000,
    FLAG_LOG_IN = 0x00000004,
    FLAG_LOGGED_IN = 0x00000005,
    FLAG_REGISTER = 0x00000006,
    FLAG_REGISTERED = 0x00000007,
    FLAG_ERROR = 0x00000009,
    FLAG_UNAUTHENTICATED = 0x0000000a,
    FLAG_ACCESS_DENIED = 0x0000000b,

    FLAG_FETCH_USERS = 0x0000000c,

    // firstly current user (A, is treated as a client) invites another user (B, is treated as a server) by sending him an invite; if B declines the invite he replies with a message containing this flag and body size = 0
    FLAG_EXCHANGE_KEYS = 0x000000a0, // if B accepts the invite, he replies with his public key, which A treats as a server key (allowing not to rewrite that part of the crypto api)
    FLAG_EXCHANGE_KEYS_DONE = 0x000000b0, // A receives the B's key, generates his key, computes shared keys and sends his public key to B; B receives it and computes shared keys too
    FLAG_EXCHANGE_HEADERS = 0x000000c0, // Next part, B generates encoder stream and sends his header to A
    FLAG_EXCHANGE_HEADERS_DONE = 0x000000d0, // A receives the B's encoder header, creates decoder and encoder, then A sends his encoder header to B
    // B receives A's header and creates decoder stream. After that, both A and B have keys and working encoders/decoders to begin an encrypted conversation

    FLAG_FILE_ASK = 0x000000e0, // firstly current user (A) sends file exchanging invite (flag_file_ask) with size == sizeof(file) to another user (B); if B accepts the invitation, he sends back to A flag_file_ask with size == sizeof(file), if he declines size == 0;
    FLAG_FILE = 0x000000f0, // secondly if B accepted the invite, A can proceed: A reads file by net_message_body_size-sized chunks, encapsulates those chunks in messages and sends them to B; B then accepts them, reads & writes those chunks to a newly created file

    FLAG_SHUTDOWN = 0x7fffffff
} Flags;

const int NET_FLAG_PROCEED = FLAG_PROCEED;

STATIC_CONST_UNSIGNED TO_SERVER = 0x7ffffffe;

const unsigned NET_USERNAME_SIZE = 16;
const unsigned NET_UNHASHED_PASSWORD_SIZE = 16;

STATIC_CONST_UNSIGNED FROM_ANONYMOUS = 0xffffffff;
STATIC_CONST_UNSIGNED FROM_SERVER = 0x7fffffff;

STATIC_CONST_UNSIGNED INVITE_ASK = 1;
STATIC_CONST_UNSIGNED INVITE_DENY = 2;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    char* host;
    unsigned port;
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    unsigned state;
    unsigned encryptedMessageSize; // constant
    NetMessageReceivedCallback onMessageReceived;
    Crypto* connectionCrypto; // client-server level encryption - different for each connection
    byte tokenAnonymous[TOKEN_SIZE]; // constant
    byte tokenServerUnsignedValue[TOKEN_UNSIGNED_VALUE_SIZE]; // constant, unencrypted but clients don't know how token is generated
    byte token[TOKEN_SIZE];
    unsigned userId;
    NetNotifierCallback onLogInResult;
    NetServiceCallback onErrorReceived;
    NetNotifierCallback onRegisterResult;
    NetCallback onDisconnected;
    NetCurrentTimeMillisGetter currentTimeMillisGetter;
    atomic int lastSentFlag;
    NetOnUsersFetched onUsersFetched;
    NetUserInfo** userInfos;
    unsigned userInfosSize;
    byte* serverKeyStub;
    atomic bool settingUpConversation;
    NetOnConversationSetUpInviteReceived onConversationSetUpInviteReceived;
    unsigned long inviteProcessingStartMillis;
    RWMutex* rwMutex;
    NetOnFileExchangeInviteReceived onFileExchangeInviteReceived;
    NetNextFileChunkSupplier nextFileChunkSupplier;
    NetNextFileChunkReceiver netNextFileChunkReceiver;
    atomic bool exchangingFile;
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

struct NetUserInfo_t {
    unsigned id;
    bool connected;
    byte name[NET_USERNAME_SIZE];
};

STATIC_CONST_UNSIGNED USER_INFO_SIZE = INT_SIZE + sizeof(bool) + NET_USERNAME_SIZE; // 21

staticAssert(sizeof(bool) == 1 && sizeof(NetUserInfo) == 21);

static void initiateSecuredConnection(const byte* serverSignPublicKey, unsigned serverSignPublicKeySize) {
    this->connectionCrypto = cryptoInit();
    assert(this->connectionCrypto);
    cryptoSetServerSignPublicKey(serverSignPublicKey, serverSignPublicKeySize);

    const unsigned signedPublicKeySize = CRYPTO_SIGNATURE_SIZE + CRYPTO_KEY_SIZE;
    byte serverSignedPublicKey[signedPublicKeySize];
    const byte* serverKeyStart = serverSignedPublicKey + CRYPTO_SIGNATURE_SIZE;

     // TODO: add timeout, wait for a constant millis count and then if no response received drop connection; add timeout to server too
    assert(SDLNet_TCP_Recv(this->socket, serverSignedPublicKey, (int) signedPublicKeySize) == (int) signedPublicKeySize);
    assert(cryptoCheckServerSignedBytes(serverSignedPublicKey, serverKeyStart, CRYPTO_KEY_SIZE));

    if (!SDL_memcmp(serverKeyStart, this->serverKeyStub, CRYPTO_KEY_SIZE)) return; // denial of service
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    if (!cryptoExchangeKeys(this->connectionCrypto, serverSignedPublicKey + CRYPTO_SIGNATURE_SIZE)) return;
    this->encryptedMessageSize = cryptoEncryptedSize(MESSAGE_SIZE);

    assert(SDLNet_TCP_Send(this->socket, cryptoClientPublicKey(this->connectionCrypto), (int) CRYPTO_KEY_SIZE) == (int) CRYPTO_KEY_SIZE);
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;

    const unsigned serverSignedCoderHeaderSize = CRYPTO_SIGNATURE_SIZE + CRYPTO_HEADER_SIZE;
    byte serverSignedCoderHeader[serverSignedCoderHeaderSize];
    const byte* serverCoderHeaderStart = serverSignedCoderHeader + CRYPTO_SIGNATURE_SIZE;

    assert(SDLNet_TCP_Recv(this->socket, serverSignedCoderHeader, (int) serverSignedCoderHeaderSize) == (int) serverSignedCoderHeaderSize);
    assert(cryptoCheckServerSignedBytes(serverSignedCoderHeader, serverCoderHeaderStart, CRYPTO_HEADER_SIZE));
    this->state = STATE_SERVER_CODER_HEADER_RECEIVED;

    byte* clientCoderHeader = cryptoInitializeCoderStreams(this->connectionCrypto, serverCoderHeaderStart);
    if (clientCoderHeader) {
        assert(SDLNet_TCP_Send(this->socket, clientCoderHeader, (int) CRYPTO_HEADER_SIZE) == (int) CRYPTO_HEADER_SIZE);
        this->state = STATE_CLIENT_CODER_HEADER_SENT;
    }

    SDL_free(clientCoderHeader);
}

bool netInit(
    const char* host,
    unsigned port,
    const byte* serverSignPublicKey,
    unsigned serverSignPublicKeySize,
    NetMessageReceivedCallback onMessageReceived,
    NetNotifierCallback onLogInResult,
    NetServiceCallback onErrorReceived,
    NetNotifierCallback onRegisterResult,
    NetCallback onDisconnected,
    NetCurrentTimeMillisGetter currentTimeMillisGetter,
    NetOnUsersFetched onUsersFetched,
    NetOnConversationSetUpInviteReceived onConversationSetUpInviteReceived,
    NetOnFileExchangeInviteReceived onFileExchangeInviteReceived,
    NetNextFileChunkSupplier nextFileChunkSupplier,
    NetNextFileChunkReceiver netNextFileChunkReceiver
) {
    assert(!this && onMessageReceived && onLogInResult && onErrorReceived && onDisconnected);

    unsigned long byteOrderChecker = 0x0123456789abcdeful; // just for notice - u & l at the end stand for unsigned long, they're not hexits (digit for hex analogue), leading 0 & x defines hex numbering system
    assert(*((byte*) &byteOrderChecker) == 0xef); // checks whether the app is running on a x64 littleEndian architecture so the byte order won't mess up data marshalling

    this = SDL_malloc(sizeof *this);

    const unsigned hostSize = SDL_strlen(host);
    this->host = SDL_malloc(hostSize);
    SDL_memcpy(this->host, host, hostSize);

    this->port = port;
    this->socket = NULL;
    this->socketSet = NULL;
    this->onMessageReceived = onMessageReceived;
    this->connectionCrypto = NULL;
    SDL_memset(this->tokenAnonymous, 0, TOKEN_SIZE);
    SDL_memset(this->tokenServerUnsignedValue, (1 << 8) - 1, TOKEN_UNSIGNED_VALUE_SIZE);
    SDL_memcpy(this->token, this->tokenAnonymous, TOKEN_SIZE); // until user is authenticated he has an anonymous token
    this->userId = FROM_ANONYMOUS;
    this->onLogInResult = onLogInResult;
    this->onErrorReceived = onErrorReceived;
    this->onRegisterResult = onRegisterResult;
    this->onDisconnected = onDisconnected;
    this->currentTimeMillisGetter = currentTimeMillisGetter;
    this->onUsersFetched = onUsersFetched;
    this->userInfos = NULL;
    this->userInfosSize = 0;
    this->serverKeyStub = SDL_calloc(CRYPTO_KEY_SIZE, sizeof(byte));
    this->settingUpConversation = false;
    this->onConversationSetUpInviteReceived = onConversationSetUpInviteReceived;
    this->inviteProcessingStartMillis = 0;
    this->rwMutex = rwMutexInit();
    this->onFileExchangeInviteReceived = onFileExchangeInviteReceived;
    this->nextFileChunkSupplier = nextFileChunkSupplier;
    this->netNextFileChunkReceiver = netNextFileChunkReceiver;
    this->exchangingFile = false;

    assert(!SDLNet_Init());

    IPaddress address;
    assert(!SDLNet_ResolveHost(&address, this->host, this->port));

#if SDL_NET_MAJOR_VERSION == 2 && SDL_NET_MINOR_VERSION == 2
    this->socket = SDLNet_TCP_Open(&address);
#elif SDL_NET_MAJOR_VERSION == 2 && SDL_NET_MINOR_VERSION == 3
    this->socket = SDLNet_TCP_OpenClient(&address);
#else
#   error "untested version"
#endif

    if (!this->socket) {
        netClean();
        return false;
    }

    this->socketSet = SDLNet_AllocSocketSet(1);
    assert(this->socketSet);
    assert(SDLNet_TCP_AddSocket(this->socketSet, this->socket) == 1);

    initiateSecuredConnection(serverSignPublicKey, serverSignPublicKeySize);

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
    switch ((int) this->lastSentFlag) {
        case FLAG_LOG_IN:
            RW_MUTEX_WRITE_LOCKED(this->rwMutex, this->state = STATE_FINISHED_WITH_ERROR;)
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

static void onUsersFetched(const Message* message);

static void processMessagesFromServer(const Message* message) {
    int cst = checkServerToken(message->token);
    assert(cst);

    switch (message->flag) {
        case FLAG_LOGGED_IN:
            RW_MUTEX_WRITE_LOCKED(this->rwMutex,
                this->state = STATE_AUTHENTICATED;
                this->userId = message->to;
                SDL_memcpy(this->token, message->body, TOKEN_SIZE);
            )
            (*(this->onLogInResult))(true);
            break;
        case FLAG_REGISTERED:
            (*(this->onRegisterResult))(true);
            break;
        case FLAG_FETCH_USERS:
            onUsersFetched(message);
            break;
        case FLAG_UNAUTHENTICATED:
        case FLAG_ERROR:
        case FLAG_ACCESS_DENIED:
            processErrors(message);
            break;
        default:
            assert(false);
    }
}

static void processConversationSetUpMessage(const Message* message) {
    assert(message->flag == FLAG_EXCHANGE_KEYS && message->size == INVITE_ASK);

    rwMutexWriteLock(this->rwMutex);
    if (this->settingUpConversation || this->exchangingFile) {
        rwMutexWriteUnlock(this->rwMutex);
        return;
    } // TODO: throw error if the mutex is being attempted to lock in write mode before it was unlocked in read mode and vice versa

    this->settingUpConversation = true;
    this->inviteProcessingStartMillis = (*(this->currentTimeMillisGetter))();
    rwMutexWriteUnlock(this->rwMutex);

    (*(this->onConversationSetUpInviteReceived))(message->from);
}

static void processFileExchangeRequestMessage(const Message* message) {
    assert(message->flag == FLAG_FILE_ASK && message->size == INT_SIZE);

    rwMutexWriteLock(this->rwMutex);
    if (this->settingUpConversation || this->exchangingFile) {
        rwMutexWriteUnlock(this->rwMutex);
        return;
    }

    this->exchangingFile = true;
    this->inviteProcessingStartMillis = (*(this->currentTimeMillisGetter))();
    rwMutexWriteUnlock(this->rwMutex);

    const unsigned fileSize = *((unsigned*) message->body);
    assert(fileSize);

    (*(this->onFileExchangeInviteReceived))(message->from, fileSize);
}

static void processMessage(const Message* message) {
    if (message->from == FROM_SERVER) {
        processMessagesFromServer(message);
        return;
    }

    switch (this->state) {
        case STATE_SECURE_CONNECTION_ESTABLISHED:
            break;
        case STATE_AUTHENTICATED:
            if (message->flag == FLAG_EXCHANGE_KEYS && message->size == INVITE_ASK)
                processConversationSetUpMessage(message);
            if (message->flag == FLAG_FILE_ASK && message->size == INT_SIZE)
                processFileExchangeRequestMessage(message);
            else if (message->flag == FLAG_PROCEED)
                (*(this->onMessageReceived))(message->timestamp, message->from, message->body, message->size);
            else
                STUB;
            break;
    }
}

static void onDisconnected(void) {
    (*(this->onDisconnected))();
    netClean();
}

static bool checkSocket(void) {
    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        const bool result = SDLNet_CheckSockets(this->socketSet, 0) == 1
            && SDLNet_SocketReady(this->socket) != 0;
    )
    return result;
}

static Message* nullable receive(void) {
    byte* buffer = SDL_malloc(this->encryptedMessageSize);

    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        const int result = SDLNet_TCP_Recv(this->socket, buffer, (int) this->encryptedMessageSize);
    )

    if (result != (int) this->encryptedMessageSize) return NULL;

    byte* decrypted = cryptoDecrypt(this->connectionCrypto, buffer, this->encryptedMessageSize, false);
    SDL_free(buffer);
    assert(decrypted);

    Message* message = unpackMessage(decrypted);
    SDL_free(decrypted);
    return message;
}

static void readReceivedMessage(void) {
    Message* message = NULL;

    if (!(message = receive()))
        onDisconnected();
    else {
        processMessage(message);
        SDL_free(message);
    }
}

void netListen(void) { // TODO: UNFREED MEMORY remains when registration fails 2 or more times !!!
    assert(this);
    while (this && checkSocket()) // read all messages that were sent during the past update frame and not only one message per update frame
        readReceivedMessage(); // checking 'this' for nullability every time despite the assertion before is needed as the module can be re-initialized during the cycle which then will cause SIGSEGV 'cause the address inside 'this' will become invalid - re-initializing after registration is the example
}

unsigned netCurrentUserId(void) {
    assert(this);
    return this->userId;
}

bool netSend(int flag, const byte* body, unsigned size, unsigned xTo) {
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
    byte* encryptedMessage = cryptoEncrypt(this->connectionCrypto, packedMessage, MESSAGE_SIZE, false);
    SDL_free(packedMessage);

    if (!encryptedMessage) return false;

    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        this->lastSentFlag = flag;
        const int bytesSent = SDLNet_TCP_Send(this->socket, encryptedMessage, (int) this->encryptedMessageSize);
    )

    SDL_free(encryptedMessage);
    return bytesSent == (int) this->encryptedMessageSize;
}

void netShutdownServer(void) {
    assert(this);

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    netSend(FLAG_SHUTDOWN, body, NET_MESSAGE_BODY_SIZE, TO_SERVER);
}

static NetUserInfo* unpackUserInfo(const byte* bytes) {
    NetUserInfo* info = SDL_malloc(sizeof *info);

    SDL_memcpy(&(info->id), bytes, INT_SIZE);
    SDL_memcpy(&(info->connected), bytes + INT_SIZE, 1);
    SDL_memcpy(&(info->name), bytes + INT_SIZE + 1, NET_USERNAME_SIZE);

    return info;
}

void netFetchUsers(void) {
    assert(this);
    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    netSend(FLAG_FETCH_USERS, body, NET_MESSAGE_BODY_SIZE, TO_SERVER);
}

unsigned netUserInfoId(const NetUserInfo* info) {
    assert(info);
    return info->id;
}

bool netUserInfoConnected(const NetUserInfo* info) {
    assert(info);
    return info->connected;
}

const byte* netUserInfoName(const NetUserInfo* info) {
    assert(info);
    return info->name;
}

static void resetUserInfos(void) { // TODO: test users list updates with large amount of items
    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        SDL_free(this->userInfos);
        this->userInfos = NULL;
        this->userInfosSize = 0;
    )
}

static void onUsersFetched(const Message* message) {
    assert(this);
    if (!(message->index)) resetUserInfos();
    rwMutexWriteLock(this->rwMutex);

    this->userInfosSize += message->size;
    assert(this->userInfosSize);
    this->userInfos = SDL_realloc(this->userInfos, this->userInfosSize * sizeof(NetUserInfo*));

    for (unsigned i = 0, j = this->userInfosSize - message->size; i < message->size; i++, j++)
        (this->userInfos)[j] = unpackUserInfo(message->body + i * USER_INFO_SIZE);

    if (message->index < message->count - 1) {
        rwMutexWriteUnlock(this->rwMutex);
        return;
    }

    (*(this->onUsersFetched))(this->userInfos, this->userInfosSize);
    for (unsigned i = 0; i < this->userInfosSize; SDL_free((this->userInfos)[i++]));

    rwMutexWriteUnlock(this->rwMutex);
    resetUserInfos();
}

static bool waitForReceiveWithTimeout(void) {
    const unsigned long startMillis = (*(this->currentTimeMillisGetter))();
    while ((*(this->currentTimeMillisGetter))() - startMillis < TIMEOUT) {
        if (checkSocket())
            return true;
    }
    return false;
}

static void finishSettingUpConversation(void)
{ RW_MUTEX_WRITE_LOCKED(this->rwMutex, this->settingUpConversation = false;) }

Crypto* nullable netCreateConversation(unsigned id) {
    assert(this);

    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        assert(!this->settingUpConversation && !this->exchangingFile);
        this->settingUpConversation = true;
    )

    byte body[NET_MESSAGE_BODY_SIZE];

    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    if (!netSend(FLAG_EXCHANGE_KEYS, body, INVITE_ASK, id)) {
        finishSettingUpConversation();
        return NULL;
    }

    Message* message;

    if (!waitForReceiveWithTimeout()) {
        finishSettingUpConversation();
        return NULL;
    }

    if (!(message = receive())
        || message->flag != FLAG_EXCHANGE_KEYS
        || message->size != CRYPTO_KEY_SIZE)
    {
        finishSettingUpConversation();
        SDL_free(message);
        return NULL;
    }

    byte akaServerPublicKey[CRYPTO_KEY_SIZE];
    SDL_memcpy(akaServerPublicKey, message->body, CRYPTO_KEY_SIZE);
    SDL_free(message);

    Crypto* crypto = cryptoInit();

    if (!cryptoExchangeKeys(crypto, akaServerPublicKey)) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, cryptoClientPublicKey(crypto), CRYPTO_KEY_SIZE);
    if (!netSend(FLAG_EXCHANGE_KEYS_DONE, body, CRYPTO_KEY_SIZE, id)) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    if (!waitForReceiveWithTimeout()) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    if (!(message = receive())
        || message->flag != FLAG_EXCHANGE_HEADERS
        || message->size != CRYPTO_HEADER_SIZE)
    {
        finishSettingUpConversation();
        SDL_free(message);
        cryptoDestroy(crypto);
        return NULL;
    }

    byte akaServerStreamHeader[CRYPTO_HEADER_SIZE];
    SDL_memcpy(akaServerStreamHeader, message->body, CRYPTO_HEADER_SIZE);
    SDL_free(message);

    byte* akaClientStreamHeader = cryptoInitializeCoderStreams(crypto, akaServerStreamHeader);
    if (!akaClientStreamHeader) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, akaClientStreamHeader, CRYPTO_HEADER_SIZE);
    SDL_free(akaClientStreamHeader);

    finishSettingUpConversation();
    if (!netSend(FLAG_EXCHANGE_HEADERS_DONE, body, CRYPTO_HEADER_SIZE, id)) {
        cryptoDestroy(crypto);
        return NULL;
    }

    return crypto;
}

static bool inviteProcessingTimeoutExceeded(void) {
    RW_MUTEX_READ_LOCKED(this->rwMutex, const unsigned long start = this->inviteProcessingStartMillis;)
    return (*(this->currentTimeMillisGetter))() - start > TIMEOUT;
}

Crypto* netReplyToPendingConversationSetUpInvite(bool accept, unsigned fromId) {
    assert(this);
    RW_MUTEX_READ_LOCKED(this->rwMutex, assert(this->settingUpConversation && !this->exchangingFile);)

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);

    if (inviteProcessingTimeoutExceeded()) {
        finishSettingUpConversation();
        return NULL;
    }

    if (!accept) {
        finishSettingUpConversation();
        netSend(FLAG_EXCHANGE_KEYS, body, INVITE_DENY, fromId);
        return NULL;
    }

    Crypto* crypto = cryptoInit();
    const byte* akaServerPublicKey = cryptoGenerateKeyPairAsServer(crypto);

    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, akaServerPublicKey, CRYPTO_KEY_SIZE);
    if (!netSend(FLAG_EXCHANGE_KEYS, body, CRYPTO_KEY_SIZE, fromId)) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    if (!waitForReceiveWithTimeout()) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    Message* message = NULL;
    if (!(message = receive())
        || message->flag != FLAG_EXCHANGE_KEYS_DONE
        || message->size != CRYPTO_KEY_SIZE)
    {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        SDL_free(message);
        return NULL;
    }

    byte akaClientPublicKey[CRYPTO_KEY_SIZE];
    SDL_memcpy(akaClientPublicKey, message->body, CRYPTO_KEY_SIZE);
    SDL_free(message);
    if (!cryptoExchangeKeysAsServer(crypto, akaClientPublicKey)) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    byte* akaServerStreamHeader = cryptoCreateEncoderAsServer(crypto);
    if (!akaServerStreamHeader) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, akaServerStreamHeader, CRYPTO_HEADER_SIZE);
    SDL_free(akaServerStreamHeader);

    if (!netSend(FLAG_EXCHANGE_HEADERS, body, CRYPTO_HEADER_SIZE, fromId)) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    if (!waitForReceiveWithTimeout()) {
        finishSettingUpConversation();
        cryptoDestroy(crypto);
        return NULL;
    }

    if (!(message = receive())
        || message->flag != FLAG_EXCHANGE_HEADERS_DONE
        || message->size != CRYPTO_HEADER_SIZE)
    {
        finishSettingUpConversation();
        SDL_free(message);
        cryptoDestroy(crypto);
        return NULL;
    }

    byte akaClientStreamHeader[CRYPTO_HEADER_SIZE];
    SDL_memcpy(akaClientStreamHeader, message->body, CRYPTO_HEADER_SIZE);
    SDL_free(message);

    finishSettingUpConversation();
    if (!cryptoCreateDecoderStreamAsServer(crypto, akaClientStreamHeader)) {
        cryptoDestroy(crypto);
        return NULL;
    }

    return crypto;
}

static void finishFileExchange(void)
{ RW_MUTEX_WRITE_LOCKED(this->rwMutex, this->exchangingFile = false;) }

bool netBeginFileExchange(unsigned toId, unsigned fileSize) { // TODO: test this
    assert(fileSize);

    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        assert(!this->settingUpConversation && !this->exchangingFile);
        this->exchangingFile = true;
    )

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    *((unsigned*) body) = fileSize;

    if (!netSend(FLAG_FILE_ASK, body, INT_SIZE, toId)) {
        finishFileExchange();
        return false;
    }

    if (!waitForReceiveWithTimeout()) {
        finishFileExchange();
        return false;
    }

    Message* message = NULL;
    if (!(message = receive())
        || message->flag != FLAG_FILE_ASK
        || *((unsigned*) message->body) != fileSize)
    {
        finishFileExchange();
        SDL_free(message);
        return false;
    }
    SDL_free(message);

    unsigned index = 0, bytesWritten;
    while ((bytesWritten = (*(this->nextFileChunkSupplier))(index++, body))) {
        if (!netSend(FLAG_FILE, body, bytesWritten, toId)) {
            finishFileExchange();
            return false;
        }
    }

    finishFileExchange();
    return true;
}

bool netReplyToFileExchangeInvite(unsigned fromId, unsigned fileSize, bool accept) {
    assert(this);
    RW_MUTEX_READ_LOCKED(this->rwMutex, assert(!this->settingUpConversation && this->exchangingFile);)

    if (inviteProcessingTimeoutExceeded()) {
        finishFileExchange();
        return false;
    }

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    if (accept) *((unsigned*) body) = fileSize;

    if (!netSend(FLAG_FILE_ASK, body, INT_SIZE, fromId)) {
        finishFileExchange();
        return false;
    }

    if (!accept) {
        finishFileExchange();
        return false;
    }

    Message* message = NULL;
    unsigned index = 0;

    while (waitForReceiveWithTimeout()
        && (message = receive())
        && message->flag == FLAG_FILE
        && message->size > 0)
    {
        assert(message->size <= NET_MESSAGE_BODY_SIZE);

        (*(this->netNextFileChunkReceiver))(fromId, index++, fileSize, message->size, message->body);

        SDL_free(message);
        message = NULL;
    }
    SDL_free(message);

    finishFileExchange();
    return true;
}

void netClean(void) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

    SDL_free(this->serverKeyStub);
    SDL_free(this->userInfos);

    if (this->connectionCrypto) cryptoDestroy(this->connectionCrypto);

    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();

    rwMutexWriteUnlock(this->rwMutex);
    rwMutexDestroy(this->rwMutex);
    SDL_free(this->host);
    SDL_free(this);
    this = NULL;
}
