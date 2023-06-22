
#include <sdl/SDL.h>
#include <assert.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "logic.h"

THIS(
    volatile bool netInitialized;
    List* usersList;
)

void logicInit(void) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
    this->usersList = renderInitUsersList();

    // TODO: test only
    for (unsigned i = 0; i < 10; i++) {
        char name[NET_USERNAME_SIZE];
        SDL_memset(name, '0' + (int) i, NET_USERNAME_SIZE - 1);
        name[NET_USERNAME_SIZE - 1] = '\0';
        listAdd(this->usersList, renderCreateUser(i, name, i % 5 == 0));
    }
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

static void onMessageReceived(const byte* message) {

}

static void onLogInResult(bool successful) {
    if (!successful) renderShowMessage("Logging in failed", true);
    else renderShowUsersList();
}

static void onErrorReceived(int flag) {
    SDL_Log("error received %d", flag);
}

static void onRegisterResult(bool successful) {
    SDL_Log("registration %s", successful ? "succeeded" : "failed");
}

static void onDisconnected(void) {
    this->netInitialized = false;
    SDL_Log("disconnected");
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

    if (!this->netInitialized) renderShowMessage("Unable to connect to the server", true); // TODO: create message queue

    if (this->netInitialized) logIn ? netLogIn(username, password) : netRegister(username, password);
}

void logicCredentialsRandomFiller(char* credentials, unsigned size) {
    assert(this);
    cryptoFillWithRandomBytes((byte*) credentials, size);
}

void logicOnLoginRegisterPageQueriedByUser(bool logIn) {
    assert(this);
    logIn ? renderShowLogIn() : renderShowRegister();
}

void logicOnUserForConversationChosen(RenderConversationChooseVariants chooseVariant) {
    SDL_Log("user for conversation chosen %d", chooseVariant);
}

void logicClean(void) {
    assert(this);
    listDestroy(this->usersList);
    if (this->netInitialized) netClean();
    SDL_free(this);
}
