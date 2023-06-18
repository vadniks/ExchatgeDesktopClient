
#include <assert.h>
#include "render/render.h"
#include "defs.h"
#include "net.h"
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
    SDL_Log("Logging in %s", successful ? "succeeded" : "failed"); // TODO: test only
    if (!successful) this->netInitialized = false;

    byte* body = SDL_calloc(netMessageBodySize(), sizeof(char));
    if (successful) netSend(0x7fffffff, body, netMessageBodySize(), 0x7ffffffe);
}

static void onErrorReceived(int flag) {
    SDL_Log("error received %d", flag);
}

static void onDisconnected(void) {
    this->netInitialized = false;
    SDL_Log("disconnected");
}

bool lifecycleInit(void) {
    renderInit();

    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->updateThreadCounter = 1;
    this->netUpdateCond = SDL_CreateCond();
    this->netUpdateLock = SDL_CreateMutex();
    this->netInitialized = netInit(&onMessageReceived, &onLogInResult, &onErrorReceived, &onDisconnected);
    this->netThread = SDL_CreateThread((int (*)(void*)) &netThread, "netThread", NULL);

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    assert(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER));

    this->threadsSynchronizerTimerId = SDL_AddTimer(
        UI_UPDATE_PERIOD,
        (unsigned (*)(unsigned, void*)) &synchronizeThreadUpdates,
        NULL
    );

    char test[16] = {'a', 'd', 'm', 'i', 'n', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // TODO: test only
//    char test[16] = {'u', 's', 'e', 'r', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//    char test[16] = {'u', 's', 'e', 'r', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (this->netInitialized) netLogIn(test, test); // TODO: fill username & password buffers with zeroes or random bytes after logging in

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

    SDL_WaitThread(this->netThread, (int[1]){});
    SDL_DestroyMutex(this->netUpdateLock);
    SDL_DestroyCond(this->netUpdateCond);

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
