
#include <assert.h>
#include "render.h"
#include "defs.h"
#include "net.h"
#include "lifecycle.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    volatile bool running;
    SDL_cond* uiUpdateCond;
    SDL_mutex* uiUpdateLock;
    SDL_TimerID uiUpdateTimerId;
    unsigned updateThreadCounter;
    SDL_cond* netUpdateCond;
    SDL_mutex* netUpdateLock;
    SDL_Thread* netThread;
)
#pragma clang diagnostic pop

static void updateSynchronized(Function action, SDL_cond* cond, SDL_mutex* lock) {
    SDL_LockMutex(lock);
    action(NULL);
    SDL_CondWait(cond, lock);
    SDL_UnlockMutex(lock);
}

static int netThread() {
    while (this->running)
        updateSynchronized((Function) &netListen, this->netUpdateCond, this->netUpdateLock);
    return 0;
}

static unsigned synchronizeThreadUpdates() {
    SDL_CondSignal(this->uiUpdateCond);

    if (this->updateThreadCounter == NET_UPDATE_PERIOD) {
        this->updateThreadCounter = 1;
        SDL_CondSignal(this->netUpdateCond);
    } else
        this->updateThreadCounter++;

    return this->running ? UI_UPDATE_PERIOD : 0;
}

bool lifecycleInit() {
    if (!netInit()) return false;
    rdInit();

    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->uiUpdateCond = SDL_CreateCond();
    this->uiUpdateLock = SDL_CreateMutex();
    this->updateThreadCounter = 1;
    this->netUpdateCond = SDL_CreateCond();
    this->netUpdateLock = SDL_CreateMutex();
    this->netThread = SDL_CreateThread((int (*)(void*)) &netThread, "netThread", NULL);

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    assert(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER));

    this->uiUpdateTimerId = SDL_AddTimer(UI_UPDATE_PERIOD, (unsigned (*)(unsigned, void*)) &synchronizeThreadUpdates, NULL);
    return true;
}

static bool processEvents() {
    SDL_Event event;
    rdInputBegan();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
        rdProcessEvent(&event);
    }

    rdInputEnded();
    return false;
}

static void stopApp() {
    this->running = false;
    SDL_CondSignal(this->netUpdateCond);
}

void lifecycleLoop() {
    while (this->running) {

        if (processEvents()) {
            stopApp();
            lifecycleClean();
            break;
        }

        updateSynchronized((Function) &rdDraw, this->uiUpdateCond, this->uiUpdateLock);
    }
}

void lifecycleClean() {
    if (!this) return;

    SDL_RemoveTimer(this->uiUpdateTimerId);

    rdClean();
    netClean();

    SDL_DestroyCond(this->uiUpdateCond);
    SDL_DestroyMutex(this->uiUpdateLock);
    SDL_WaitThread(this->netThread, (int[1]){});
    SDL_DestroyMutex(this->netUpdateLock);
    SDL_DestroyCond(this->netUpdateCond);

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
