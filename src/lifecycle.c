
#include <assert.h>
#include <unistd.h>
#include "render/render.h"
#include "defs.h"
#include "net.h"
#include "crypto.h"
#include "lifecycle.h"

static const unsigned UI_UPDATE_PERIOD = 1000 / 60;
static const unsigned NET_UPDATE_PERIOD = 60 / 15;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    volatile bool running;
    SDL_TimerID threadsSynchronizerTimerId;
    unsigned updateThreadCounter;
    SDL_cond* netUpdateCond;
    SDL_mutex* netUpdateLock;
    SDL_Thread* netThread;
    volatile bool netInitialized;
)
#pragma clang diagnostic pop

static void updateSynchronized(Function action, SDL_cond* cond, SDL_mutex* lock) {
    SDL_LockMutex(lock);
    action(NULL);
    SDL_CondWait(cond, lock);
    SDL_UnlockMutex(lock);
}

static void netThread(void) {
    while (this->running)
        if (this->netInitialized)
            updateSynchronized((Function) &netListen, this->netUpdateCond, this->netUpdateLock);
}

static unsigned synchronizeThreadUpdates(void) {
    if (this->updateThreadCounter == NET_UPDATE_PERIOD) {
        this->updateThreadCounter = 1;
        SDL_CondSignal(this->netUpdateCond);
    } else
        this->updateThreadCounter++;

    return this->running ? UI_UPDATE_PERIOD : 0;
}

static void onMessageReceived(const byte* message) {} // TODO

static void onLogInResult(bool successful) {
    SDL_Log("logging in %s", successful ? "succeeded" : "failed"); // TODO: test only

    byte* body = SDL_calloc(netMessageBodySize(), sizeof(char));
    if (successful) netSend(0x7fffffff, body, netMessageBodySize(), 0x7ffffffe);
    SDL_free(body);
}

static void onErrorReceived(int flag) {
    SDL_Log("error received %d", flag);
}

static void onRegisterResult(bool successful) {
    SDL_Log("registration %s", successful ? "succeeded" : "failed");
}
int a = 0; // TODO: test only
static void onDisconnected(void) { // TODO: run netClean() after calling this callback
    this->netInitialized = false;
    SDL_Log("disconnected");

    if (a == 0) { // TODO: test only
        this->netInitialized = true;
        this->netInitialized = netInit(&onMessageReceived, &onLogInResult, &onErrorReceived, &onRegisterResult, &onDisconnected);
        char test[16] = {'a', 'd', 'm', 'i', 'n', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        netLogIn(test, test);
    }
    a++;
}

static void onCredentialsReceived(
    const char* username,
    const char* password,
    bool logIn
) {// TODO: expose net module's flags in it's header
    SDL_Log("credentials received %s %s %c", username, password, logIn ? 't' : 'f');

    if (!this->netInitialized) this->netInitialized = netInit(
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected
    );

    if (this->netInitialized) logIn ? netLogIn(username, password) : netRegister(username, password);
}

static void onUiDelayEnded(void) {
    sleep(1);
    renderShowLogIn();
}

static void delayUi(void) { // causes render module to show splash page until logIn page is queried one second later
    SDL_Thread* uiInitialDelayThread = SDL_CreateThread(
        (int (*)(void*)) &onUiDelayEnded,
        "uiInitialDelayThread",
        NULL
    );
    SDL_DetachThread(uiInitialDelayThread);
}

static void credentialsRandomFiller(char* credentials, unsigned size)
{ cryptoFillWithRandomBytes((byte*) credentials, size); }

static void onLoginRegisterPageQueriedByUser(bool logIn) {
    SDL_Log("%s", logIn ? "log in page requested" : "register page requested");
}

bool lifecycleInit(void) { // TODO: expose net module's flags in it's header
    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->updateThreadCounter = 1;
    this->netUpdateCond = SDL_CreateCond();
    this->netUpdateLock = SDL_CreateMutex();
    this->netInitialized = false;
    this->netThread = SDL_CreateThread((int (*)(void*)) &netThread, "netThread", NULL);

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    assert(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER));

    renderInit(
        USERNAME_SIZE,
        PASSWORD_SIZE,
        &onCredentialsReceived,
        &credentialsRandomFiller,
        &onLoginRegisterPageQueriedByUser
    );
    delayUi();

    this->threadsSynchronizerTimerId = SDL_AddTimer(
        UI_UPDATE_PERIOD,
        (unsigned (*)(unsigned, void*)) &synchronizeThreadUpdates,
        NULL
    );

//    char test[16] = {'a', 'd', 'm', 'i', 'n', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // TODO: test only
//    char test[16] = {'u', 's', 'e', 'r', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//    char test[16] = {'n', 'e', 'w', '0', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//
//    if (this->netInitialized) { // TODO: test only
//        char test[16] = {'u', 's', 'e', 'r', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//        netLogIn(test, test);
//    }

    return true;
}

static bool processEvents(void) {
    SDL_Event event;
    renderInputBegan();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
        renderProcessEvent(&event);
    }

    renderInputEnded();
    return false;
}

static void stopApp(void) {
    this->running = false;
    SDL_CondSignal(this->netUpdateCond);
}

void lifecycleLoop(void) {
    while (this->running) {

        if (processEvents()) {
            stopApp();
            lifecycleClean();
            break;
        }

        renderDraw();
    }
}

void lifecycleClean(void) {
    if (!this) return;

    SDL_RemoveTimer(this->threadsSynchronizerTimerId);

    renderClean();
    if (this->netInitialized) netClean();

    SDL_WaitThread(this->netThread, NULL);
    SDL_DestroyMutex(this->netUpdateLock);
    SDL_DestroyCond(this->netUpdateCond);

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
