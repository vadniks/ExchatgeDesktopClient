
#include <sdl/SDL.h>
#include <assert.h>
#include <time.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "user.h"
#include "conversationMessage.h"
#include "lifecycle.h"
#include "database/database.h"
#include "logic.h"

typedef enum : unsigned {
    STATE_UNAUTHENTICATED = 0,
    STATE_AWAITING_AUTHENTICATION = 1,
    STATE_AUTHENTICATED = 2,
    STATE_EXCHANGING_MESSAGES = 3
} States;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    volatile bool netInitialized;
    List* usersList; // <User*>
    List* messagesList; // <ConversationMessage*>
    bool adminMode;
    volatile unsigned state;
    char* currentUserName; // TODO: the server shouldn't know groups in which users are, so group information stays on clients, the server just transmits messages
    unsigned toUserId; // the id of the user, the current user (logged in via this client) wanna speak to
    volatile bool databaseInitialized;
    Crypto* nullable currentConversationCrypto;
)
#pragma clang diagnostic pop

static void parseArguments(unsigned argc, const char** argv) {
    assert(argc <= 1 || argc == 2 && argv[1]); // 'cause argv[0] is path to the executable everytime

    if (argc <= 1) {
        this->adminMode = false;
        return;
    }

    const unsigned patternSize = 7;
    const char pattern[patternSize] = "--admin"; // Just unlock access to admin operations, they're executed on the server after it verifies the caller, so verification on the client side is unnecessary
    this->adminMode = !SDL_memcmp(argv[1], pattern, patternSize);
}

void logicInit(unsigned argc, const char** argv) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
    this->usersList = userInitList();
    this->messagesList = conversationMessageInitList();
    parseArguments(argc, argv);
    this->state = STATE_UNAUTHENTICATED;
    this->currentUserName = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
    this->databaseInitialized = false;
    this->currentConversationCrypto = NULL;

    lifecycleAsync((LifecycleAsyncActionFunction) &renderShowLogIn, NULL, 1000);
}

bool logicIsAdminMode(void) {
    assert(this);
    return this->adminMode;
}

void logicNetListen(void) {
    if (!this) return; // logic module might haven't been initialized by the time lifecycle module's netThread has started periodically calling this function to update connection
    if (!this->netInitialized) return;
    netListen();
}

const List* logicUsersList(void) {
    assert(this);
    return this->usersList;
}

const List* logicMessagesList(void) {
    assert(this);
    return this->messagesList;
}

static const User* nullable findUser(unsigned id) {
    const unsigned size = listSize(this->usersList);
    const User* user = NULL;

    for (unsigned i = 0; i < size; i++) {
        user = listGet(this->usersList, i);
        assert(user);

        if (user->id == id)
            return user;
    }

    return NULL;
}

static void onMessageReceived(unsigned long timestamp, unsigned fromId, const byte* encryptedMessage, unsigned encryptedSize) {
    assert(this && this->databaseInitialized && this->currentConversationCrypto);

    const User* user = findUser(fromId);
    assert(user);

    const unsigned size = encryptedSize - cryptoEncryptedSize(0);
    assert(size > 0 && size <= logicUnencryptedMessageBodySize());

    byte* message = cryptoDecrypt(this->currentConversationCrypto, encryptedMessage, encryptedSize, false);
    assert(message);

    DatabaseMessage* dbMessage = databaseMessageCreate(timestamp, &fromId, (byte*) message, size);
    databaseAddMessage(dbMessage);
    databaseMessageDestroy(dbMessage);

    listAdd(this->messagesList, conversationMessageCreate(timestamp, user->name, NET_USERNAME_SIZE, (const char*) message, size));
    SDL_free(message);
}

static void onLogInResult(bool successful) {
    assert(this);
    if (successful) {
        this->state = STATE_AUTHENTICATED;
        netFetchUsers();
    } else {
        this->state = STATE_UNAUTHENTICATED;
        renderShowLogIn();
        renderShowSystemError();
        renderHideInfiniteProgressBar();
    }
}

static void onErrorReceived(__attribute_maybe_unused__ int flag) {
    assert(this);
    renderHideInfiniteProgressBar();
    renderShowSystemError();
}

static void onRegisterResult(bool successful) {
    assert(this);
    successful ? renderShowRegistrationSucceededSystemMessage() : renderShowSystemError();
    renderHideInfiniteProgressBar();
}

static void onDisconnected(void) { // TODO: forbid using username 'admin' more than one time on the server side
    assert(this);

    this->netInitialized = false;
    this->state = STATE_UNAUTHENTICATED;

    renderHideInfiniteProgressBar();
    renderShowLogIn();
    renderShowDisconnectedError();
}

static void onUsersFetched(NetUserInfo** infos, unsigned size) {
    assert(this && this->databaseInitialized);

    NetUserInfo* info;
    for (unsigned i = 0, id; i < size; i++) {
        info = infos[i];
        id = netUserInfoId(info);

        if (id != netCurrentUserId())
            listAdd(this->usersList,userCreate(
                id,
                (const char*) netUserInfoName(info),
                NET_USERNAME_SIZE,
                databaseConversationExists(id),
                netUserInfoConnected(info)
            ));
        else {
            SDL_memcpy(this->currentUserName, netUserInfoName(info), NET_USERNAME_SIZE);
            renderSetWindowTitle(this->currentUserName);
        }
    }

    renderHideInfiniteProgressBar();
    renderShowUsersList(this->currentUserName);
}

static void replyToConversationSetUpInvite(unsigned* fromId) {
    assert(fromId);
    unsigned xFromId = *fromId;
    SDL_free(fromId);

    const User* user = findUser(xFromId);
    assert(user);

    Crypto* crypto = netReplyToPendingConversationSetUpInvite(renderShowInviteDialog(user->name), xFromId);
    assert(!(this->currentConversationCrypto));
    this->currentConversationCrypto = crypto;

    if (crypto)
        databaseAddConversation(xFromId, crypto),
        renderShowConversation(user->name);
    else
        renderShowUnableToCreateConversation();

    renderSetControlsBlocking(false);
    renderHideInfiniteProgressBar();

    SDL_Log("establishing secured connection with inviter %s", crypto ? "succeeded" : "failed"); // TODO: test only
}

static void onConversationSetUpInviteReceived(unsigned fromId) {
    unsigned* xFromId = SDL_malloc(sizeof(int));
    *xFromId = fromId;
    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);
    lifecycleAsync((LifecycleAsyncActionFunction) &replyToConversationSetUpInvite, xFromId, 0);
}

static void processCredentials(void** data) {
    const char* username = data[0];
    const char* password = data[1];
    bool logIn = *((bool*) data[2]);

    if (!databaseInit((byte*) password, NET_UNHASHED_PASSWORD_SIZE, NET_USERNAME_SIZE, NET_MESSAGE_BODY_SIZE)) { // password that's used to sign in also used to encrypt messages & other stuff in the database
        databaseClean();
        renderShowUnableToDecryptDatabaseError();
        renderHideInfiniteProgressBar();
        goto cleanup;
    } else
        this->databaseInitialized = true;

    assert(this);
    this->netInitialized = netInit(
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected,
        &logicCurrentTimeMillis,
        &onUsersFetched,
        &onConversationSetUpInviteReceived
    );

    if (!this->netInitialized) {
        this->state = STATE_UNAUTHENTICATED;
        renderShowUnableToConnectToTheServerError();
        renderHideInfiniteProgressBar();
    }
    if (this->netInitialized) logIn ? netLogIn(username, password) : netRegister(username, password);

    cleanup:
    logicCredentialsRandomFiller(data[0], NET_USERNAME_SIZE);
    logicCredentialsRandomFiller(data[1], NET_UNHASHED_PASSWORD_SIZE);

    SDL_free(data[0]);
    SDL_free(data[1]);
    SDL_free(data[2]);
    SDL_free(data);
}

void logicOnCredentialsReceived(const char* username, const char* password, bool logIn) {
    assert(this);
    if (this->state != STATE_UNAUTHENTICATED) return;
    assert(!this->netInitialized);

    this->state = STATE_AWAITING_AUTHENTICATION;
    renderShowInfiniteProgressBar();

    void** data = SDL_malloc(3 * sizeof(void*));
    data[0] = SDL_malloc(NET_USERNAME_SIZE * sizeof(char));
    data[1] = SDL_malloc(NET_UNHASHED_PASSWORD_SIZE * sizeof(char));
    data[2] = SDL_malloc(sizeof(bool));

    SDL_memcpy(data[0], username, NET_USERNAME_SIZE);
    SDL_memcpy(data[1], password, NET_UNHASHED_PASSWORD_SIZE);
    *((bool*) data[2]) = logIn;

    lifecycleAsync((LifecycleAsyncActionFunction) &processCredentials, data, 0);
}

void logicCredentialsRandomFiller(char* credentials, unsigned size) {
    assert(this);
    cryptoFillWithRandomBytes((byte*) credentials, size);
}

void logicOnLoginRegisterPageQueriedByUser(bool logIn) {
    assert(this);
    logIn ? renderShowLogIn() : renderShowRegister();
}

static void createConversation(unsigned* id) {
    assert(id && this->databaseInitialized && !databaseConversationExists(*id));

    Crypto* crypto = netCreateConversation(*id);
    assert(!(this->currentConversationCrypto));
    this->currentConversationCrypto = crypto;

    const User* user = findUser(*id);
    assert(user);

    if (crypto)
        databaseAddConversation(*id, crypto),
        renderShowConversation(user->name);
    else
        renderShowUnableToCreateConversation();

    SDL_free(id);
    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);

    SDL_Log("establishing secured connection with invited user %s", crypto ? "succeeded" : "failed"); // TODO: test only
}

static void startConversation(unsigned id) {
    const User* user = findUser(id);
    assert(user);

    if (!(user->online)) {
        renderShowUserIsOfflineError();
        return;
    }

    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);

    unsigned* xId = SDL_malloc(sizeof(int));
    *xId = id;
    lifecycleAsync((LifecycleAsyncActionFunction) &createConversation, xId, 0);
}

static void continueConversation(unsigned id) {

}

static void deleteConversation(unsigned id) {

}

void logicOnUserForConversationChosen(unsigned id, RenderConversationChooseVariants chooseVariant) {
    assert(this);
    this->state = STATE_EXCHANGING_MESSAGES;
    this->toUserId = id;
    listClear(this->messagesList);

    switch (chooseVariant) {
        case RENDER_START_CONVERSATION:
            startConversation(id);
            break;
        case RENDER_CONTINUE_CONVERSATION:
            continueConversation(id);
            break;
        case RENDER_DELETE_CONVERSATION:
            deleteConversation(id);
            break;
        default:
            assert(false);
    }
}

void logicOnServerShutdownRequested(void) {
    assert(this);
    if (!this->netInitialized) return;

    renderShowInfiniteProgressBar();
    lifecycleAsync((LifecycleAsyncActionFunction) &netShutdownServer, NULL, 0);
}

void logicOnReturnFromConversationPageRequested(void) {
    assert(this && this->databaseInitialized);

    assert(this->currentConversationCrypto);
    SDL_free(this->currentConversationCrypto);
    this->currentConversationCrypto = NULL;

    renderShowUsersList(this->currentUserName);
}

static void utos(char* buffer, unsigned length, unsigned number) { // unsigned to string
    byte* digits = NULL;
    unsigned digitsSize = 0;

    while (number > 0) {
        digits = SDL_realloc(digits, ++digitsSize * sizeof(char));
        digits[digitsSize - 1] = number % 10;
        number /= 10;
    }

    if (length < digitsSize) {
        assert(false);
        //return;
    }

    for (unsigned i = digitsSize, j = 0; i < length; i++, j++)
        buffer[j] = '0' + 0;
    for (unsigned i = length - digitsSize, j = digitsSize - 1; i < length; i++, j--)
        buffer[i] = (char) ('0' + digits[j]);

    SDL_free(digits);
}

static void monthToName(char* buffer, unsigned monthIndex) {
    assert(monthIndex < 12);
    const char months[12][3] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    SDL_memcpy(buffer, months[monthIndex], 3);
}

char* logicMillisToDateTime(unsigned long millis) {
    millis /= 1000;
    struct tm* tm = localtime((long*) &millis);
    assert(tm);

    // 'hh:mm:ss mmm-dd-yyyy'  20 + \0 = 21
    char* text = SDL_calloc(21, sizeof(char));

    utos(text, 2, tm->tm_hour);
    text[2] = ':';
    utos(text + 3, 2, tm->tm_min);
    text[5] = ':';
    utos(text + 6, 2, tm->tm_sec);
    text[8] = ' ';

    monthToName(text + 9, tm->tm_mon);
    text[12] = '-';
    utos(text + 13, 2, tm->tm_mday);
    text[15] = '-';
    utos(text + 16, 4, tm->tm_year + 1900);
    text[20] = 0;

    return text;
}

unsigned long logicCurrentTimeMillis(void) {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return timespec.tv_sec * (unsigned) 1e3f + timespec.tv_nsec / (unsigned) 1e6f;
}

static void sendMessage(void** params) { // TODO: block controls while sending/receiving a message
    assert(this && params && this->databaseInitialized && this->currentConversationCrypto);

    unsigned size = *((unsigned*) params[1]), encryptedSize = cryptoEncryptedSize(size);
    assert(size && encryptedSize <= NET_MESSAGE_BODY_SIZE);

    const byte* text = params[0];
    DatabaseMessage* dbMessage = databaseMessageCreate(logicCurrentTimeMillis(), NULL, text, size);
    databaseAddMessage(dbMessage);
    databaseMessageDestroy(dbMessage);

    byte* encryptedText = cryptoEncrypt(this->currentConversationCrypto, text, size, false);

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, encryptedText, encryptedSize);
    SDL_free(encryptedText);

    netSend(NET_FLAG_PROCEED, body, encryptedSize, this->toUserId); // TODO: test all

    SDL_free(params[0]);
    SDL_free(params[1]);
    SDL_free(params);
}

void logicOnSendClicked(const char* text, unsigned size) {
    assert(this);
    void** params = SDL_malloc(2 * sizeof(void*));

    params[0] = SDL_malloc(size * sizeof(char));
    SDL_memcpy(params[0], text, size);

    params[1] = SDL_malloc(sizeof(int));
    *((unsigned*) params[1]) = size;

    listAdd(this->messagesList, conversationMessageCreate(logicCurrentTimeMillis(), NULL, 0, text, size));
    lifecycleAsync((LifecycleAsyncActionFunction) &sendMessage, params, 0);
}

void logicOnUpdateUsersListClicked(void) {
    assert(this);

    listClear(this->usersList);
    renderShowUsersList(this->currentUserName);

    lifecycleAsync((LifecycleAsyncActionFunction) &netFetchUsers, NULL, 0);
}

unsigned logicUnencryptedMessageBodySize(void) { return NET_MESSAGE_BODY_SIZE - cryptoEncryptedSize(0); } // 928 - 17 = 911

void logicClean(void) {
    assert(this);

    SDL_free(this->currentConversationCrypto);
    if (this->databaseInitialized) databaseClean();

    SDL_free(this->currentUserName);

    listDestroy(this->usersList);
    listDestroy(this->messagesList);

    if (this->netInitialized) netClean();
    SDL_free(this);
}
