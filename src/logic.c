
#include <sdl/SDL.h>
#include <assert.h>
#include <time.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "logic.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    volatile bool netInitialized;
    List* usersList;
    LogicAsyncTask asyncTask;
    LogicDelayedTask delayedTask;
    List* messagesList;
    bool adminMode;
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

    // TODO: test only
    for (unsigned i = 0; i < 100; i++) {
        char name[NET_USERNAME_SIZE];
        SDL_memset(name, '0' + (int) i, NET_USERNAME_SIZE - 1);
        name[NET_USERNAME_SIZE - 1] = '\0';
        listAdd(this->usersList, renderCreateUser(i, name, i % 5 == 0, i % 2 == 0));
    }
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

static void onMessageReceived(const byte* message) {

}

static void onLogInResult(bool successful) {
    if (successful)
        renderShowUsersList();
    else {
        renderShowLogIn();
        renderShowSystemError();
    }
}

static void onErrorReceived(__attribute_maybe_unused__ int flag) {
    renderHideInfiniteProgressBar();
    renderShowSystemError();
}

static void onRegisterResult(bool successful) {
    SDL_Log("registration %s", successful ? "succeeded" : "failed");
}

static void onDisconnected(void) { // TODO: forbid using username 'admin' more than one time on the server side
    this->netInitialized = false;
    renderHideInfiniteProgressBar();
    renderShowLogIn();
    renderShowDisconnectedSystemMessage();
}

static void processCredentials(void** data) {
    const char* username = data[0];
    const char* password = data[1];
    bool logIn = *((bool*) data[2]);

    assert(this); // TODO: thread data race possible when app has been exited but the thread still executes - might cause dereference error
    this->netInitialized = netInit( // TODO: to solve this issue save references to all threads and wait for them in module's clean function
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected
    );

    if (!this->netInitialized) renderShowUnableToConnectToTheServerSystemMessage();
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
    assert(this && !this->netInitialized);
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
    SDL_Log("user for conversation chosen %d for user %u", chooseVariant, id);

    if (chooseVariant == RENDER_START_CONVERSATION || chooseVariant == RENDER_CONTINUE_CONVERSATION) {
        for (unsigned i = 0; i < 100; i++) { // TODO: test only
            char text[NET_MESSAGE_BODY_SIZE];
            SDL_memset(text, '0' + (int) i, NET_MESSAGE_BODY_SIZE);
            listAdd(this->messagesList, renderCreateMessage(i, i % 5 == 0, text, NET_MESSAGE_BODY_SIZE));
        }

        char title[NET_USERNAME_SIZE]; // TODO: test only
        SDL_memcpy(title, "Conversation", 12);
        SDL_memset(title + 12, 0, NET_USERNAME_SIZE - 12);
        renderShowConversation(title);
    }
}

void logicOnServerShutdownRequested(void) {
    assert(this);
    if (!this->netInitialized) return;

    renderShowInfiniteProgressBar();
    (*(this->asyncTask))(&netShutdownServer);
}

void logicOnReturnFromConversationPageRequested(void) {
    assert(this);
    renderShowUsersList();
}

static void utos(char* buffer, unsigned length, unsigned number) {
    byte* digits = NULL;
    unsigned digitsSize = 0;

    while (number > 0) {
        digits = SDL_realloc(digits, digitsSize++ * sizeof(char));
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
}

char* logicMillisToDateTime(unsigned long millis) {
    millis /= 1000;
    struct tm* tm = localtime((long*) &millis);
    assert(tm);

    // 'ss:mm:hh mm-dd-yyyy'  19 + \0 = 20
    char* text = SDL_calloc(20, sizeof(char));

    utos(text, 2, tm->tm_sec);
    text[2] = ':';
    utos(text + 3, 2, tm->tm_min);
    text[5] = ':';
    utos(text + 6, 2, tm->tm_hour);
    text[8] = ' ';

    utos(text + 9, 2, tm->tm_mon);
    text[11] = '-';
    utos(text + 12, 2, tm->tm_mday);
    text[14] = '-';
    utos(text + 15, 4, tm->tm_year);
    text[19] = 0;

    return text;
}

static unsigned long currentTimeMillis(void) {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return timespec.tv_sec * (unsigned) 1e3f + timespec.tv_nsec / (unsigned) 1e6f;
}

void logicClean(void) {
    char a[4] = {0}; // TODO: test only
    utos(a, 4, 123);
    SDL_Log("%s %s", a, logicMillisToDateTime(currentTimeMillis()));

    assert(this);
    listDestroy(this->usersList);
    listDestroy(this->messagesList);
    if (this->netInitialized) netClean();
    SDL_free(this);
}
