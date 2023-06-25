
#include <sdl/SDL.h>
#include <assert.h>
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
        listAdd(this->usersList, renderCreateUser(i, name, i % 5 == 0));
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
    if (successful) {
        renderHideSystemMessage();
        renderShowUsersList();
    } else {
        renderShowLogIn();
        renderShowSystemError();
    }
}

static void onErrorReceived(int flag) {
    SDL_Log("error received %d", flag);
}

static void onRegisterResult(bool successful) {
    SDL_Log("registration %s", successful ? "succeeded" : "failed");
}

static void onDisconnected(void) {
    this->netInitialized = false;
    renderShowLogIn();
    renderShowDisconnectedSystemMessage();
}

void logicOnCredentialsReceived(const char* username, const char* password, bool logIn) {
    assert(this && !this->netInitialized);
    this->netInitialized = netInit(
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected
    );

    if (!this->netInitialized) renderShowUnableToConnectToTheServerSystemMessage(); // TODO: create text queue
    if (this->netInitialized) logIn ? netLogIn(username, password) : netRegister(username, password);
}

void logicCredentialsRandomFiller(char* credentials, unsigned size) {
    assert(this);
    cryptoFillWithRandomBytes((byte*) credentials, size);
}

void logicOnLoginRegisterPageQueriedByUser(bool logIn) {
    assert(this);
    renderHideSystemMessage();
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
    if (this->netInitialized) netShutdownServer();
}

void logicClean(void) {
    assert(this);
    listDestroy(this->usersList);
    listDestroy(this->messagesList);
    if (this->netInitialized) netClean();
    SDL_free(this);
}
