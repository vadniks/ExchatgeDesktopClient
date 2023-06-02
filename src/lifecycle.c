
#include <time.h>
#include "render.h"
#include "defs.h"
#include "net.h"
#include "lifecycle.h"

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

static void updateSynchronized(function action, SDL_cond* cond, SDL_mutex* lock) {
    SDL_LockMutex(lock);
    action(NULL);
    SDL_CondWait(cond, lock);
    SDL_UnlockMutex(lock);
}

static int netThread(__attribute_maybe_unused__ void* _) {
    while (this->running)
        updateSynchronized((function) &ntListen, this->netUpdateCond, this->netUpdateLock);
    return 0;
}

static unsigned uiUpdate(
    __attribute_maybe_unused__ unsigned _,
    __attribute_maybe_unused__ void* _2
) {
    SDL_CondSignal(this->uiUpdateCond);

    if (this->updateThreadCounter == (unsigned) NET_UPDATE_PERIOD) {
        this->updateThreadCounter = 1;
        SDL_CondSignal(this->netUpdateCond);
    } else
        this->updateThreadCounter++;

    return this->running ? UI_UPDATE_PERIOD : 0;
}

bool lcInit() {
    if (!ntInit()) return false;
    rdInit();

    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->uiUpdateCond = SDL_CreateCond();
    this->uiUpdateLock = SDL_CreateMutex();
    this->updateThreadCounter = 1;
    this->netUpdateCond = SDL_CreateCond();
    this->netUpdateLock = SDL_CreateMutex();
    this->netThread = SDL_CreateThread(&netThread, "netThread", NULL);

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    this->uiUpdateTimerId = SDL_AddTimer(UI_UPDATE_PERIOD, &uiUpdate, NULL);
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

void lcLoop() {
    while (this->running) {
        if (processEvents()) {
            this->running = false;
            lcClean();
            break;
        }

        updateSynchronized((function) &rdDraw, this->uiUpdateCond, this->uiUpdateLock);
    }
}

void lcClean() {
    if (!this) return;

    SDL_RemoveTimer(this->uiUpdateTimerId);

    rdClean();
    ntClean();

    SDL_DestroyCond(this->uiUpdateCond);
    SDL_DestroyMutex(this->uiUpdateLock);
    SDL_DetachThread(this->netThread);
    SDL_DestroyMutex(this->netUpdateLock);
    SDL_DestroyCond(this->netUpdateCond);

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
