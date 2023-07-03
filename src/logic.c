
#include <sdl/SDL.h>
#include <assert.h>
#include <time.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "logic.h"

STATIC_CONST_UNSIGNED STATE_UNAUTHENTICATED = 0;
STATIC_CONST_UNSIGNED STATE_AWAITING_AUTHENTICATION = 1;
STATIC_CONST_UNSIGNED STATE_AUTHENTICATED = 2;
STATIC_CONST_UNSIGNED STATE_EXCHANGING_MESSAGES = 3;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    volatile bool netInitialized;
    List* usersList; // <RenderUser*>
    LogicAsyncTask asyncTask;
    LogicDelayedTask delayedTask;
    List* messagesList; // <RenderMessage*>
    bool adminMode;
    volatile unsigned state;
    char* currentUserName; // TODO: the server shouldn't know groups in which users are, so group information stays on clients, the server just transmits messages
    unsigned toUserId; // the id of the user, the current user (logged in via this client) wanna speak to
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

void logicInit(unsigned argc, const char** argv, LogicAsyncTask asyncTask, LogicDelayedTask delayedTask) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
    this->usersList = renderInitUsersList();
    this->asyncTask = asyncTask;
    this->delayedTask = delayedTask;
    this->messagesList = renderInitMessagesList();
    parseArguments(argc, argv);
    this->state = STATE_UNAUTHENTICATED;
    this->currentUserName = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
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

static const RenderUser* nullable findUser(unsigned id) {
    const unsigned size = listSize(this->usersList);
    const RenderUser* user = NULL;

    for (unsigned i = 0; i < size; i++) {
        user = listGet(this->usersList, i);
        assert(user);

        if (renderUserId(user) == id)
            return user;
    }

    return NULL;
}

static void onMessageReceived(unsigned long timestamp, unsigned fromId, const byte* message, unsigned size) {
    assert(this);

    const RenderUser* user = findUser(fromId);
    assert(user);

    SDL_Log("message received from %u %s", fromId, renderUserName(user)); // TODO: test only

    listAdd(this->messagesList, renderCreateMessage(timestamp, renderUserName(user), (const char*) message, size));
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
}

static void onDisconnected(void) { // TODO: forbid using username 'admin' more than one time on the server side
    assert(this);

    this->netInitialized = false;
    this->state = STATE_UNAUTHENTICATED;

    renderHideInfiniteProgressBar();
    renderShowLogIn();
    renderShowDisconnectedSystemMessage();
}

static void onUsersFetched(NetUserInfo** infos, unsigned size) {
    assert(this);

    NetUserInfo* info;
    for (unsigned i = 0, id; i < size; i++) {
        info = infos[i];
        id = netUserInfoId(info);

        if (id != netCurrentUserId())
            listAdd(this->usersList,renderCreateUser(
                id,
                (const char*) netUserInfoName(info),
                /*TODO: client side's business whether a conversation with a particular user exists*/i % 5 == 0,
                netUserInfoConnected(info)
            ));
        else
            SDL_memcpy(this->currentUserName, netUserInfoName(info), NET_USERNAME_SIZE);
    }

    renderShowUsersList(this->currentUserName);
}

static void processCredentials(void** data) {
    const char* username = data[0];
    const char* password = data[1];
    bool logIn = *((bool*) data[2]);

    assert(this);
    this->netInitialized = netInit(
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected,
        &logicCurrentTimeMillis,
        &onUsersFetched
    );

    if (!this->netInitialized) {
        this->state = STATE_UNAUTHENTICATED;
        renderShowUnableToConnectToTheServerSystemMessage();
    }
    if (this->netInitialized) logIn ? netLogIn(username, password) : netRegister(username, password);

    logicCredentialsRandomFiller(data[0], NET_USERNAME_SIZE);
    logicCredentialsRandomFiller(data[1], NET_UNHASHED_PASSWORD_SIZE);

    SDL_free(data[0]);
    SDL_free(data[1]);
    SDL_free(data[2]);
    SDL_free(data);

    renderHideInfiniteProgressBar();
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

    SDL_DetachThread(SDL_CreateThread((int (*)(void*)) &processCredentials, "credentialsProcessThread", data));
}

void logicCredentialsRandomFiller(char* credentials, unsigned size) {
    assert(this);
    cryptoFillWithRandomBytes((byte*) credentials, size);
}

void logicOnLoginRegisterPageQueriedByUser(bool logIn) {
    assert(this);
    logIn ? renderShowLogIn() : renderShowRegister();
}

void logicOnUserForConversationChosen(unsigned id, RenderConversationChooseVariants chooseVariant) {
    assert(this);
    this->state = STATE_EXCHANGING_MESSAGES;
    this->toUserId = id;
    listClear(this->messagesList);

    const RenderUser* user = findUser(id);
    assert(user);

    SDL_Log("user for conversation chosen %d for user %u", chooseVariant, id);

    if (chooseVariant == RENDER_START_CONVERSATION || chooseVariant == RENDER_CONTINUE_CONVERSATION)
        renderShowConversation(renderUserName(user));
}

void logicOnServerShutdownRequested(void) {
    assert(this);
    if (!this->netInitialized) return;

    renderShowInfiniteProgressBar();
    (*(this->asyncTask))(&netShutdownServer);
}

void logicOnReturnFromConversationPageRequested(void) {
    assert(this);
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

    // 'ss:mm:hh mmm-dd-yyyy'  20 + \0 = 21
    char* text = SDL_calloc(21, sizeof(char));

    utos(text, 2, tm->tm_sec);
    text[2] = ':';
    utos(text + 3, 2, tm->tm_min);
    text[5] = ':';
    utos(text + 6, 2, tm->tm_hour);
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

static void sendMessage(void** params) {
    assert(this && params);

    unsigned size = *((unsigned*) params[1]);
    byte body[NET_MESSAGE_BODY_SIZE];

    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, (const char*) params[0], size);

    netSend(NET_FLAG_PROCEED, body, size, this->toUserId);

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

    listAdd(this->messagesList, renderCreateMessage(logicCurrentTimeMillis(), NULL, text, size));
    SDL_DetachThread(SDL_CreateThread((SDL_ThreadFunction) &sendMessage, "sendThread", params));
}

void logicOnUpdateUsersListClicked(void) {
    assert(this);

    listClear(this->usersList);
    renderShowUsersList(this->currentUserName);

    (*(this->asyncTask))(&netFetchUsers);
}

void logicClean(void) {
    assert(this);

    SDL_free(this->currentUserName);

    listDestroy(this->usersList);
    listDestroy(this->messagesList);

    if (this->netInitialized) netClean();
    SDL_free(this);
}
