
#include <sdl/SDL.h>
#include <assert.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "logic.h"

THIS(
    volatile bool netInitialized;
)

void logicInit(void) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
}

void logicNetListen(void) {
    assert(this);
    if (!this->netInitialized) return;
    netListen();
}

static void onMessageReceived(const byte* message) {

}

static void onLogInResult(bool successful) {
    if (!successful) renderShowMessage("Logging in failed", true);
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

void logicClean(void) {
    assert(this);
    if (this->netInitialized) netClean();
    SDL_free(this);
}
