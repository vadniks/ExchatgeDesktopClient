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

#include <SDL.h>
#include <SDL_net.h>
#include <assert.h>
#include <endian.h>
#include "collections/list.h"
#include "collections/queue.h"
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

STATIC_CONST_UNSIGNED MAX_MESSAGE_SIZE = 1 << 8; // 256
STATIC_CONST_UNSIGNED TOKEN_TRAILING_SIZE = 16;
STATIC_CONST_UNSIGNED TOKEN_UNSIGNED_VALUE_SIZE = 2 * INT_SIZE; // 8
STATIC_CONST_UNSIGNED TOKEN_SIZE = TOKEN_UNSIGNED_VALUE_SIZE + 40 + TOKEN_TRAILING_SIZE; // 64
STATIC_CONST_UNSIGNED MESSAGE_HEAD_SIZE = INT_SIZE * 6 + LONG_SIZE + TOKEN_SIZE; // 96
const unsigned NET_MAX_MESSAGE_BODY_SIZE = MAX_MESSAGE_SIZE - MESSAGE_HEAD_SIZE; // 160

STATIC_CONST_UNSIGNED long TIMEOUT = 5000; // in milliseconds

typedef enum : int {
    FLAG_PROCEED = 0x00000000,
    FLAG_BROADCAST = 0x10000000,

    FLAG_LOG_IN = 0x00000004,
    FLAG_LOGGED_IN = 0x00000005,

    FLAG_REGISTER = 0x00000006,
    FLAG_REGISTERED = 0x00000007,

    FLAG_ERROR = 0x00000009,

    FLAG_FETCH_USERS = 0x0000000c,
    FLAG_FETCH_MESSAGES = 0x0000000d,

    // firstly current user (A, is treated as a client) invites another user (B, is treated as a server) by sending him an invite; if B declines the invite he replies with a message containing this flag and body size = 0
    FLAG_EXCHANGE_KEYS = 0x000000a0, // if B accepts the invite, he replies with his public key, which A treats as a server key (allowing not to rewrite that part of the crypto api)
    FLAG_EXCHANGE_KEYS_DONE = 0x000000b0, // A receives the B's key, generates his key, computes shared keys and sends his public key to B; B receives it and computes shared keys too
    FLAG_EXCHANGE_HEADERS = 0x000000c0, // Next part, B generates encoder stream and sends his header to A
    FLAG_EXCHANGE_HEADERS_DONE = 0x000000d0, // A receives the B's encoder header, creates decoder and encoder, then A sends his encoder header to B
    // B receives A's header and creates decoder stream. After that, both A and B have keys and working encoders/decoders to begin an encrypted conversation

    // TODO: create new Crypto (new cipher instance) for each file to be exchanged to avoid overuse of the main (conversation's) one as one cipher can handle only 0x7fffffff encryption operations
    FLAG_FILE_ASK = 0x000000e0, // firstly current user (A) sends file exchanging invite (flag_file_ask) with size == sizeof(file) to another user (B); if B accepts the invitation, he sends back to A flag_file_ask with size == sizeof(file), if he declines size == 0;
    FLAG_FILE = 0x000000f0, // secondly if B accepted the invite, A can proceed: A reads file by net_message_body_size-sized chunks, encapsulates those chunks in messages and sends them to B; B then accepts them, reads & writes those chunks to a newly created file

    // TODO: rename FLAG_FILE to FLAG_FILE_CHUNK

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

const unsigned NET_MAX_FILENAME_SIZE = NET_MAX_MESSAGE_BODY_SIZE - 40; // 120 // 40 = INT_SIZE + CRYPTO_HASH_SIZE + INT_SIZE

staticAssert(NET_MAX_FILENAME_SIZE < NET_MAX_MESSAGE_BODY_SIZE);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    char* host;
    unsigned port;
    TCPsocket socket;
    SDLNet_SocketSet socketSet;
    atomic unsigned state;
    NetOnMessageReceived onMessageReceived;
    CryptoCoderStreams* connectionCoderStreams;
    byte tokenAnonymous[TOKEN_SIZE]; // constant
    byte tokenServerUnsignedValue[TOKEN_UNSIGNED_VALUE_SIZE]; // constant, unencrypted but clients don't know how token is generated
    byte token[TOKEN_SIZE];
    unsigned userId;
    NetOnLogInResult onLogInResult;
    NetOnErrorReceived onErrorReceived;
    NetOnRegisterResult onRegisterResult;
    NetOnDisconnected onDisconnected;
    NetCurrentTimeMillisGetter currentTimeMillisGetter;
    NetOnUsersFetched onUsersFetched;
    List* userInfosList;
    byte* serverKeyStub;
    atomic bool settingUpConversation;
    NetOnConversationSetUpInviteReceived onConversationSetUpInviteReceived;
    unsigned long inviteProcessingStartMillis;
    RWMutex* rwMutex;
    NetOnFileExchangeInviteReceived onFileExchangeInviteReceived;
    NetNextFileChunkSupplier nextFileChunkSupplier;
    NetNextFileChunkReceiver netNextFileChunkReceiver;
    atomic bool exchangingFile;
    Queue* conversationSetupMessages;
    Queue* fileExchangeMessages;
    atomic bool fetchingUsers;
    atomic bool fetchingMessages; // TODO: deprecated: practically unused, doesn't work properly
    NetOnNextMessageFetched onNextMessageFetched;
    atomic bool ignoreUsualMessages; // ignore usual messages from other users (with flag proceed) while updating user infos or while re-fetching messages
    NetOnBroadcastMessageReceived onBroadcastMessageReceived;
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
    byte* nullable body; // payload
} Message;

struct NetUserInfo_t {
    unsigned id;
    bool connected;
    byte name[NET_USERNAME_SIZE];
};

STATIC_CONST_UNSIGNED USER_INFO_SIZE = INT_SIZE + sizeof(bool) + NET_USERNAME_SIZE; // 21

staticAssert(sizeof(bool) == 1 && sizeof(NetUserInfo) == 21);

static bool checkSocket(void);

static bool waitForReceiveWithTimeout(void) {
    const unsigned long startMillis = (*(this->currentTimeMillisGetter))();
    while ((*(this->currentTimeMillisGetter))() - startMillis < TIMEOUT) {
        if (checkSocket())
            return true;
    }
    return false;
}

static void initiateSecuredConnection(const byte* serverSignPublicKey, unsigned serverSignPublicKeySize, CryptoKeys* connectionKeys) {
    this->connectionCoderStreams = cryptoCoderStreamsInit();

    cryptoSetServerSignPublicKey(serverSignPublicKey, serverSignPublicKeySize);

    const unsigned signedPublicKeySize = CRYPTO_SIGNATURE_SIZE + CRYPTO_KEY_SIZE;
    byte serverSignedPublicKey[signedPublicKeySize];
    const byte* serverKeyStart = serverSignedPublicKey + CRYPTO_SIGNATURE_SIZE;

    // TODO: add timeout to server too
    if (!waitForReceiveWithTimeout()) return;

    if (SDLNet_TCP_Recv(
        this->socket,
        serverSignedPublicKey,
        (int) signedPublicKeySize
    ) != (int) signedPublicKeySize) return;
    assert(cryptoCheckServerSignedBytes(serverSignedPublicKey, serverKeyStart, CRYPTO_KEY_SIZE));

    if (!SDL_memcmp(serverKeyStart, this->serverKeyStub, CRYPTO_KEY_SIZE)) return; // denial of service
    this->state = STATE_SERVER_PUBLIC_KEY_RECEIVED;

    if (!cryptoExchangeKeys(connectionKeys, serverSignedPublicKey + CRYPTO_SIGNATURE_SIZE)) return;

    if (SDLNet_TCP_Send(
        this->socket,
        cryptoClientPublicKey(connectionKeys),
        (int) CRYPTO_KEY_SIZE
    ) != (int) CRYPTO_KEY_SIZE) return;
    this->state = STATE_CLIENT_PUBLIC_KEY_SENT;

    const unsigned serverSignedCoderHeaderSize = CRYPTO_SIGNATURE_SIZE + CRYPTO_HEADER_SIZE;
    byte serverSignedCoderHeader[serverSignedCoderHeaderSize];
    const byte* serverCoderHeaderStart = serverSignedCoderHeader + CRYPTO_SIGNATURE_SIZE;

    if (!waitForReceiveWithTimeout()) return;
    if (SDLNet_TCP_Recv(
        this->socket,
        serverSignedCoderHeader,
        (int) serverSignedCoderHeaderSize
    ) != (int) serverSignedCoderHeaderSize) return;

    assert(cryptoCheckServerSignedBytes(serverSignedCoderHeader, serverCoderHeaderStart, CRYPTO_HEADER_SIZE));
    this->state = STATE_SERVER_CODER_HEADER_RECEIVED;

    byte* clientCoderHeader = cryptoInitializeCoderStreams(
        connectionKeys,
        this->connectionCoderStreams,
        serverCoderHeaderStart
    );

    if (clientCoderHeader) {
        if (SDLNet_TCP_Send(this->socket, clientCoderHeader, (int) CRYPTO_HEADER_SIZE) == (int) CRYPTO_HEADER_SIZE)
            this->state = STATE_CLIENT_CODER_HEADER_SENT;
    }

    SDL_free(clientCoderHeader);
}

static void destroyMessage(Message* nullable msg) {
    if (!msg) return;
    SDL_free(msg->body);
    SDL_free(msg);
}

bool netInit(
    const char* host,
    unsigned port,
    const byte* serverSignPublicKey,
    unsigned serverSignPublicKeySize,
    NetOnMessageReceived onMessageReceived,
    NetOnLogInResult onLogInResult,
    NetOnErrorReceived onErrorReceived,
    NetOnRegisterResult onRegisterResult,
    NetOnDisconnected onDisconnected,
    NetCurrentTimeMillisGetter currentTimeMillisGetter,
    NetOnUsersFetched onUsersFetched,
    NetOnConversationSetUpInviteReceived onConversationSetUpInviteReceived,
    NetOnFileExchangeInviteReceived onFileExchangeInviteReceived,
    NetNextFileChunkSupplier nextFileChunkSupplier,
    NetNextFileChunkReceiver netNextFileChunkReceiver,
    NetOnNextMessageFetched onNextMessageFetched,
    NetOnBroadcastMessageReceived onBroadcastMessageReceived
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
    this->connectionCoderStreams = NULL;
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
    this->userInfosList = listInit(&SDL_free);
    this->serverKeyStub = SDL_calloc(CRYPTO_KEY_SIZE, sizeof(byte));
    this->settingUpConversation = false;
    this->onConversationSetUpInviteReceived = onConversationSetUpInviteReceived;
    this->inviteProcessingStartMillis = 0;
    this->rwMutex = rwMutexInit();
    this->onFileExchangeInviteReceived = onFileExchangeInviteReceived;
    this->nextFileChunkSupplier = nextFileChunkSupplier;
    this->netNextFileChunkReceiver = netNextFileChunkReceiver;
    this->exchangingFile = false;
    this->conversationSetupMessages = queueInitExtra((QueueDeallocator) &destroyMessage, currentTimeMillisGetter);
    this->fileExchangeMessages = queueInitExtra((QueueDeallocator) &destroyMessage, currentTimeMillisGetter);
    this->fetchingUsers = false;
    this->fetchingMessages = false;
    this->onNextMessageFetched = onNextMessageFetched;
    this->ignoreUsualMessages = false;
    this->onBroadcastMessageReceived = onBroadcastMessageReceived;

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

    CryptoKeys* connectionKeys = cryptoKeysInit();
    initiateSecuredConnection(serverSignPublicKey, serverSignPublicKeySize, connectionKeys);
    cryptoKeysDestroy(connectionKeys);

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

void netLogIn(const char* username, const char* password) {
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

static inline unsigned wholeMessageBytesSize(unsigned size) { return MESSAGE_HEAD_SIZE + size; } // replace the pointer to body with the actual body

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

    assert(msg->size <= NET_MAX_MESSAGE_BODY_SIZE);
    if (msg->size) {
        msg->body = SDL_malloc(msg->size);
        SDL_memcpy(msg->body, buffer + MESSAGE_HEAD_SIZE, msg->size);
    } else
        msg->body = NULL;

    return msg;
}

static byte* packMessage(const Message* msg) {
    assert(!msg->body && !msg->size || msg->body && msg->size && msg->size <= NET_MAX_MESSAGE_BODY_SIZE);
    byte* buffer = SDL_malloc(wholeMessageBytesSize(msg->size));

    SDL_memcpy(buffer, &(msg->flag), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE, &(msg->timestamp), LONG_SIZE);
    SDL_memcpy(buffer + INT_SIZE + LONG_SIZE, &(msg->size), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 2 + LONG_SIZE, &(msg->index), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 3 + LONG_SIZE, &(msg->count), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 4 + LONG_SIZE, &(msg->from), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 5 + LONG_SIZE, &(msg->to), INT_SIZE);
    SDL_memcpy(buffer + INT_SIZE * 6 + LONG_SIZE, &(msg->token), TOKEN_SIZE);

    if (msg->body && msg->size) SDL_memcpy(buffer + MESSAGE_HEAD_SIZE, msg->body, msg->size);

    return buffer;
}

static void processErrors(const Message* message) { // TODO: send operation flag in error message's payload instead of remembering the last operation's flag
    assert(message->size == INT_SIZE && message->body);
    switch (*((int*) message->body)) {
        case FLAG_LOG_IN:
            this->state = STATE_FINISHED_WITH_ERROR;
            (*(this->onLogInResult))(false);
            break;
        case FLAG_REGISTER:
            this->state = STATE_FINISHED_WITH_ERROR;
            (*(this->onRegisterResult))(false);
            break;
        case FLAG_FETCH_MESSAGES: // ignore
            break;
        default:
            (*(this->onErrorReceived))(message->flag);
            break;
    }
}

static void onNextUsersBundleFetched(const Message* message);
static void onNextMessageFetched(const Message* message);
static void onEmptyMessagesFetchReplyReceived(const Message* message);

static void processMessagesFromServer(const Message* message) {
    const bool checked = checkServerToken(message->token);
    assert(checked);

    switch (message->flag) {
        case FLAG_LOGGED_IN:
            assert(message->body && message->size);
            this->state = STATE_AUTHENTICATED;

            RW_MUTEX_WRITE_LOCKED(this->rwMutex,
                this->userId = message->to;
                SDL_memcpy(this->token, message->body, TOKEN_SIZE);
            )

            (*(this->onLogInResult))(true);
            break;
        case FLAG_REGISTERED:
            (*(this->onRegisterResult))(true);
            break;
        case FLAG_FETCH_USERS: // TODO: raise assert if the users list hasn't been fully formed or add timeout
            onNextUsersBundleFetched(message);
            break;
        case FLAG_ERROR:
            processErrors(message);
            break;
        case FLAG_FETCH_MESSAGES:
            onEmptyMessagesFetchReplyReceived(message);
            break;
        case FLAG_BROADCAST:
            assert(message->body && message->size);
            (*(this->onBroadcastMessageReceived))(message->body, message->size);
            break;
        default:
            assert(false);
    }
}

static void processConversationSetUpMessage(const Message* message) {
    assert(message->body && message->size);
    assert(message->flag == FLAG_EXCHANGE_KEYS && message->size == INVITE_ASK);

    if (this->settingUpConversation || this->exchangingFile)
        return;

    this->settingUpConversation = true;
    this->inviteProcessingStartMillis = (*(this->currentTimeMillisGetter))();

    (*(this->onConversationSetUpInviteReceived))(message->from);
}

static inline unsigned fileExchangeRequestInitialSize(void)
{ return INT_SIZE + CRYPTO_HASH_SIZE + INT_SIZE + NET_MAX_FILENAME_SIZE; }

static void processFileExchangeRequestMessage(const Message* message) {
    assert(message->body && message->size);
    assert(message->flag == FLAG_FILE_ASK && message->size == fileExchangeRequestInitialSize());

    if (this->settingUpConversation || this->exchangingFile)
        return;

    this->exchangingFile = true;
    this->inviteProcessingStartMillis = (*(this->currentTimeMillisGetter))();

    const unsigned fileSize = *((unsigned*) message->body);
    assert(fileSize);

    byte hash[CRYPTO_HASH_SIZE];
    SDL_memcpy(hash, message->body + INT_SIZE, CRYPTO_HASH_SIZE);

    const unsigned filenameSize = *((unsigned*) message->body + INT_SIZE + CRYPTO_HASH_SIZE);
    char filename[filenameSize];
    SDL_memcpy(filename, message->body + INT_SIZE + CRYPTO_HASH_SIZE + INT_SIZE, filenameSize);

    (*(this->onFileExchangeInviteReceived))(message->from, fileSize, hash, filename, filenameSize);
}

static Message* copyMessage(const Message* message) {
    assert(message->body && message->size || !message->body && !message->size);

    Message* new = SDL_malloc(sizeof(Message));
    SDL_memcpy(new, message, sizeof *new);

    if (message->size) {
        new->body = SDL_malloc(message->size);
        SDL_memcpy(new->body, message->body, message->size);
    }

    return new;
}

static void processMessage(const Message* message) {
    if (message->from == FROM_SERVER) {
        processMessagesFromServer(message);
        return;
    }

    assert(this->state == STATE_AUTHENTICATED);

    switch (message->flag) {
        case FLAG_EXCHANGE_KEYS:
            if (message->size == INVITE_ASK) {
                processConversationSetUpMessage(message);
                break;
            }
            fallthrough
        case FLAG_EXCHANGE_KEYS_DONE: fallthrough
        case FLAG_EXCHANGE_HEADERS: fallthrough
        case FLAG_EXCHANGE_HEADERS_DONE:
            queuePush(this->conversationSetupMessages, copyMessage(message));
            break;
        case FLAG_FILE_ASK:
            if (message->size == fileExchangeRequestInitialSize()) {
                assert(message->body);
                processFileExchangeRequestMessage(message);
                break;
            }
            fallthrough
        case FLAG_FILE:
            queuePush(this->fileExchangeMessages, copyMessage(message));
            break;
        case FLAG_PROCEED:
            assert(message->body && message->size);
            if (!this->fetchingUsers && !this->fetchingMessages && !this->ignoreUsualMessages) // messages from other users can be lost here, but as we now have a re-fetching messages mechanism, this is no longer a problem
                (*(this->onMessageReceived))(message->timestamp, message->from, message->body, message->size);
            break;
        case FLAG_FETCH_MESSAGES:
            onNextMessageFetched(message);
            break;
        default:
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

static unsigned encryptedMessageMaxSize(void) { return cryptoEncryptedSize(MAX_MESSAGE_SIZE); }

static bool receivePart(void* buffer, unsigned targetSize) { // parts: size - first part, encrypted message - second part
    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        const int result = SDLNet_TCP_Recv(this->socket, buffer, (int) targetSize);
    )
    return result == (int) targetSize; // Recv returns zero on socket was disconnection or error appearing
}

static Message* nullable receive(void) {
    unsigned size = 0;
    if (!receivePart(&size, INT_SIZE)) return NULL; // disconnected
    assert(size && size <= encryptedMessageMaxSize());

    byte buffer[size];
    if (!receivePart(buffer, size)) return NULL;

    byte* decrypted = cryptoDecrypt(this->connectionCoderStreams, buffer, size, false);
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
        destroyMessage(message);
    }
}

void netListen(void) {
    assert(this);
    while (this && checkSocket()) // read all messages that were sent during the past update frame and not only one message per update frame
        readReceivedMessage(); // checking 'this' for nullability every time despite the assertion before is needed as the module can be re-initialized during the cycle which then will cause SIGSEGV 'cause the address inside 'this' will become invalid - re-initializing after registration is the example
}

unsigned netCurrentUserId(void) {
    assert(this);
    return this->userId;
}

static bool sendBytes(const void* buffer, unsigned targetSize) {
    RW_MUTEX_WRITE_LOCKED(this->rwMutex,
        const int bytesSent = SDLNet_TCP_Send(this->socket, buffer, (int) targetSize);
    )
    return bytesSent == (int) targetSize;
}

bool netSend(int flag, const byte* nullable body, unsigned size, unsigned xTo) {
    assert(this);
    assert(body && size && size <= NET_MAX_MESSAGE_BODY_SIZE || !body && !size && flag != FLAG_PROCEED && flag != FLAG_BROADCAST);

    Message message = {
        flag,
        (*(this->currentTimeMillisGetter))(),
        size,
        0,
        1,
        this->userId,
        xTo,
        {0},
        size ? (byte*) body : NULL
    };
    SDL_memcpy(&(message.token), this->token, TOKEN_SIZE);

    const unsigned packedSize = wholeMessageBytesSize(size);

    byte* packedMessage = packMessage(&message);
    byte* encryptedMessage = cryptoEncrypt(
        this->connectionCoderStreams,
        packedMessage,
        packedSize,
        false
    );
    SDL_free(packedMessage);

    if (!encryptedMessage) return false;
    const unsigned encryptedSize = cryptoEncryptedSize(packedSize);
    assert(encryptedSize <= cryptoEncryptedSize(MAX_MESSAGE_SIZE));

    byte buffer[INT_SIZE + encryptedSize];
    *((unsigned*) buffer) = encryptedSize;
    SDL_memcpy(buffer + INT_SIZE, encryptedMessage, encryptedSize);
    SDL_free(encryptedMessage);

    return sendBytes(buffer, sizeof buffer);
}

void netShutdownServer(void) {
    assert(this);
    netSend(FLAG_SHUTDOWN, NULL, 0, TO_SERVER);
}

static NetUserInfo* unpackUserInfo(const byte* bytes) {
    NetUserInfo* info = SDL_malloc(sizeof *info);

    SDL_memcpy(&(info->id), bytes, INT_SIZE);
    SDL_memcpy(&(info->connected), bytes + INT_SIZE, 1);
    SDL_memcpy(&(info->name), bytes + INT_SIZE + 1, NET_USERNAME_SIZE);

    return info;
}

void netFetchUsers(void) {
    assert(this && !this->fetchingUsers && !this->fetchingMessages);
    this->fetchingUsers = true;
    netSend(FLAG_FETCH_USERS, NULL, 0, TO_SERVER);
}

void netSendBroadcast(const byte* text, unsigned size) {
    assert(this && size);
    netSend(FLAG_BROADCAST, text, size, TO_SERVER);
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

NetUserInfo* netUserInfoCopy(const NetUserInfo* info) {
    assert(info);
    NetUserInfo* new = SDL_malloc(sizeof *new);

    new->id = info->id;
    new->connected = info->connected;
    SDL_memcpy(new->name, info->name, NET_USERNAME_SIZE);

    return new;
}

void netUserInfoDestroy(NetUserInfo* info) { SDL_free(info); }

void netSetIgnoreUsualMessages(bool ignore) {
    assert(this);
    this->ignoreUsualMessages = ignore;
}

void netFetchMessages(unsigned id, unsigned long afterTimestamp) {
    assert(this && !this->fetchingUsers && !this->fetchingMessages);
    this->fetchingMessages = true;

    byte body[1 + LONG_SIZE + INT_SIZE] = {0};

    body[0] = 1;
    *((unsigned long*) &(body[1])) = afterTimestamp;
    *((unsigned*) &(body[1 + LONG_SIZE])) = id;

    netSend(FLAG_FETCH_MESSAGES, body, sizeof body, TO_SERVER);
}

static void onNextMessageFetched(const Message* message) {
    assert(this && this->fetchingMessages);
    assert(message->body && message->size);
    const bool last = message->index == message->count - 1;

    (*(this->onNextMessageFetched))(message->from, message->timestamp, message->size, message->body, last);
    if (last) this->fetchingMessages = false;
}

static void onEmptyMessagesFetchReplyReceived(const Message* message) {
    assert(this && this->fetchingMessages);
    assert(message->body && message->size);
    assert(message->count == 1);

    (*(this->onNextMessageFetched))(*(unsigned*) (message->body + 1 + LONG_SIZE), message->timestamp, 0, NULL, true);
    this->fetchingMessages = false;
}

static void onNextUsersBundleFetched(const Message* message) {
    assert(this && this->fetchingUsers);
    assert(message->body && message->size && message->size <= NET_MAX_MESSAGE_BODY_SIZE);
    if (!(message->index)) listClear(this->userInfosList); // TODO: test with large amount of elements & test with sleep()

    for (unsigned i = 0; i < message->size; i += USER_INFO_SIZE)
        listAddBack(this->userInfosList, unpackUserInfo(message->body + i));

    if (message->index < message->count - 1) return; // not the last bundle

    (*(this->onUsersFetched))(this->userInfosList);
    listClear(this->userInfosList);
    this->fetchingUsers = false;
}

static void finishSettingUpConversation(void) {
    queueClear(this->conversationSetupMessages);
    this->settingUpConversation = false;
}

CryptoCoderStreams* nullable netCreateConversation(unsigned id) { // TODO: start receiving messages from users only after all users were fetched and assert that the sender's id is found locally
    assert(this);

    assert(!this->settingUpConversation && !this->exchangingFile);
    this->settingUpConversation = true;
    queueClear(this->conversationSetupMessages);

    if (!netSend(FLAG_EXCHANGE_KEYS, (byte[INVITE_ASK]) {0}, INVITE_ASK, id)) {
        finishSettingUpConversation();
        return NULL;
    }

    Message* message;

    if (!(message = queueWaitAndPop(this->conversationSetupMessages, (int) TIMEOUT))
        || message->flag != FLAG_EXCHANGE_KEYS
        || message->size != CRYPTO_KEY_SIZE
        || !message->body)
    {
        finishSettingUpConversation();
        destroyMessage(message);
        return NULL;
    }

    byte akaServerPublicKey[CRYPTO_KEY_SIZE];
    SDL_memcpy(akaServerPublicKey, message->body, CRYPTO_KEY_SIZE);
    destroyMessage(message);

    CryptoKeys* keys = cryptoKeysInit();

    if (!cryptoExchangeKeys(keys, akaServerPublicKey)) {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        return NULL;
    }

    if (!netSend(FLAG_EXCHANGE_KEYS_DONE, cryptoClientPublicKey(keys), CRYPTO_KEY_SIZE, id)) {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        return NULL;
    }

    if (!(message = queueWaitAndPop(this->conversationSetupMessages, (int) TIMEOUT))
        || message->flag != FLAG_EXCHANGE_HEADERS
        || message->size != CRYPTO_HEADER_SIZE
        || !message->body)
    {
        finishSettingUpConversation();
        destroyMessage(message);
        cryptoKeysDestroy(keys);
        return NULL;
    }

    byte akaServerStreamHeader[CRYPTO_HEADER_SIZE];
    SDL_memcpy(akaServerStreamHeader, message->body, CRYPTO_HEADER_SIZE);
    destroyMessage(message);

    CryptoCoderStreams* coderStreams = cryptoCoderStreamsInit();
    byte* akaClientStreamHeader = cryptoInitializeCoderStreams(keys, coderStreams, akaServerStreamHeader);
    cryptoKeysDestroy(keys);

    if (!akaClientStreamHeader) {
        finishSettingUpConversation();
        cryptoCoderStreamsDestroy(coderStreams);
        return NULL;
    }

    const bool result = netSend(FLAG_EXCHANGE_HEADERS_DONE, akaClientStreamHeader, CRYPTO_HEADER_SIZE, id);
    SDL_free(akaClientStreamHeader);
    finishSettingUpConversation();

    if (!result) {
        cryptoCoderStreamsDestroy(coderStreams);
        return NULL;
    }

    return coderStreams;
}

static inline bool inviteProcessingTimeoutExceeded(void)
{ return (*(this->currentTimeMillisGetter))() - this->inviteProcessingStartMillis > TIMEOUT; }

CryptoCoderStreams* nullable netReplyToConversationSetUpInvite(bool accept, unsigned fromId) { // TODO: create a dashboard page with some analysis for admin
    assert(this);
    assert(this->settingUpConversation && !this->exchangingFile);
    queueClear(this->conversationSetupMessages);

    if (inviteProcessingTimeoutExceeded()) {
        finishSettingUpConversation();
        return NULL;
    }

    if (!accept) {
        finishSettingUpConversation();
        netSend(FLAG_EXCHANGE_KEYS, (byte[INVITE_DENY]) {0}, INVITE_DENY, fromId);
        return NULL;
    }

    CryptoKeys* keys = cryptoKeysInit();
    const byte* akaServerPublicKey = cryptoGenerateKeyPairAsServer(keys);

    if (!netSend(FLAG_EXCHANGE_KEYS, akaServerPublicKey, CRYPTO_KEY_SIZE, fromId)) {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        return NULL;
    }

    Message* message = NULL;
    if (!(message = queueWaitAndPop(this->conversationSetupMessages, (int) TIMEOUT))
        || message->flag != FLAG_EXCHANGE_KEYS_DONE
        || message->size != CRYPTO_KEY_SIZE
        || !message->body)
    {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        destroyMessage(message);
        return NULL;
    }

    byte akaClientPublicKey[CRYPTO_KEY_SIZE];
    SDL_memcpy(akaClientPublicKey, message->body, CRYPTO_KEY_SIZE);
    destroyMessage(message);
    if (!cryptoExchangeKeysAsServer(keys, akaClientPublicKey)) {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        return NULL;
    }

    CryptoCoderStreams* coderStreams = cryptoCoderStreamsInit();
    byte* akaServerStreamHeader = cryptoCreateEncoderAsServer(keys, coderStreams);

    if (!akaServerStreamHeader) {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        cryptoCoderStreamsDestroy(coderStreams);
        return NULL;
    }

    const bool result = netSend(FLAG_EXCHANGE_HEADERS, akaServerStreamHeader, CRYPTO_HEADER_SIZE, fromId);
    SDL_free(akaServerStreamHeader);

    if (!result) {
        finishSettingUpConversation();
        cryptoKeysDestroy(keys);
        cryptoCoderStreamsDestroy(coderStreams);
        return NULL;
    }

    if (!(message = queueWaitAndPop(this->conversationSetupMessages, (int) TIMEOUT))
        || message->flag != FLAG_EXCHANGE_HEADERS_DONE
        || message->size != CRYPTO_HEADER_SIZE
        || !message->body)
    {
        finishSettingUpConversation();
        destroyMessage(message);
        cryptoKeysDestroy(keys);
        cryptoCoderStreamsDestroy(coderStreams);
        return NULL;
    }

    byte akaClientStreamHeader[CRYPTO_HEADER_SIZE];
    SDL_memcpy(akaClientStreamHeader, message->body, CRYPTO_HEADER_SIZE);
    destroyMessage(message);

    finishSettingUpConversation();

    const bool decoderCreated = cryptoCreateDecoderStreamAsServer(keys, coderStreams, akaClientStreamHeader);
    cryptoKeysDestroy(keys);

    if (!decoderCreated) {
        cryptoCoderStreamsDestroy(coderStreams);
        return NULL;
    }

    return coderStreams;
}

static void finishFileExchanging(void) {
    queueClear(this->fileExchangeMessages);
    this->exchangingFile = false;
}

bool netBeginFileExchange(unsigned toId, unsigned fileSize, const byte* hash, const char* filename, unsigned filenameSize) {
    assert(this);
    assert(fileSize);
    assert(filenameSize <= NET_MAX_FILENAME_SIZE);

    assert(!this->settingUpConversation && !this->exchangingFile);
    this->exchangingFile = true;
    queueClear(this->fileExchangeMessages);

    byte body[fileExchangeRequestInitialSize()];
    SDL_memset(body, 0, NET_MAX_MESSAGE_BODY_SIZE);

    *((unsigned*) body) = fileSize;
    SDL_memcpy(body + INT_SIZE, hash, CRYPTO_HASH_SIZE);
    *((unsigned*) body + INT_SIZE + CRYPTO_HASH_SIZE) = filenameSize;
    SDL_memcpy(body + INT_SIZE + CRYPTO_HASH_SIZE + INT_SIZE, filename, filenameSize);

    if (!netSend(FLAG_FILE_ASK, body, fileExchangeRequestInitialSize(), toId)) {
        finishFileExchanging();
        return false;
    }

    Message* message = NULL;
    if (!(message = queueWaitAndPop(this->fileExchangeMessages, (int) TIMEOUT))
        || message->flag != FLAG_FILE_ASK
        || message->size != INT_SIZE
        || !message->body
        || *((unsigned*) message->body) != fileSize)
    {
        finishFileExchanging();
        destroyMessage(message);
        return false;
    }
    destroyMessage(message);

    unsigned index = 0, bytesWritten;
    while ((bytesWritten = (*(this->nextFileChunkSupplier))(index++, body))) {
        assert(bytesWritten <= NET_MAX_MESSAGE_BODY_SIZE);

        if (!netSend(FLAG_FILE, body, bytesWritten, toId)) {
            finishFileExchanging();
            return false;
        }
    }

    finishFileExchanging();
    return true;
}

bool netReplyToFileExchangeInvite(unsigned fromId, unsigned fileSize, bool accept) {
    assert(this);
    assert(!this->settingUpConversation && this->exchangingFile);
    queueClear(this->fileExchangeMessages);

    if (inviteProcessingTimeoutExceeded()) {
        finishFileExchanging();
        return false;
    }

    if (!accept) fileSize = 0;
    if (!netSend(FLAG_FILE_ASK, (byte*) &fileSize, INT_SIZE, fromId)) {
        finishFileExchanging();
        return false;
    }

    if (!accept) {
        finishFileExchanging();
        return false;
    }

    Message* message = NULL; // TODO: add possibility for admin to disable/enable file exchanging and set the maximum/minimum fie size
    unsigned index = 0;

    unsigned long lastReceivedChunkMillis = (*(this->currentTimeMillisGetter))();
    while ((message = queueWaitAndPop(this->fileExchangeMessages, (int) TIMEOUT))) { // TODO: add progress bar (not infinite, the real one with %)
        if ((*(this->currentTimeMillisGetter))() - lastReceivedChunkMillis >= TIMEOUT)
            break;

        if (message->flag == FLAG_FILE && message->size > 0 && message->body)
            lastReceivedChunkMillis = (*(this->currentTimeMillisGetter))();
        else {
            destroyMessage(message);
            message = NULL;
            continue;
        }

        assert(message->size <= NET_MAX_MESSAGE_BODY_SIZE);
        (*(this->netNextFileChunkReceiver))(fromId, index++, message->size, message->body);

        destroyMessage(message);
        message = NULL;
    }
    destroyMessage(message);

    finishFileExchanging();
    return true; // TODO: check index == __initial_message__->count here and returns check result + this will shorten the loading time
}

void netClean(void) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

    queueDestroy(this->conversationSetupMessages);
    queueDestroy(this->fileExchangeMessages);

    SDL_free(this->serverKeyStub);
    listDestroy(this->userInfosList);

    if (this->connectionCoderStreams) cryptoCoderStreamsDestroy(this->connectionCoderStreams);

    SDLNet_FreeSocketSet(this->socketSet);
    SDLNet_TCP_Close(this->socket);
    SDLNet_Quit();

    rwMutexWriteUnlock(this->rwMutex);
    rwMutexDestroy(this->rwMutex);
    SDL_free(this->host);
    SDL_free(this);
    this = NULL;
}
