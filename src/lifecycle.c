
#include <stdbool.h>
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
)

inline static void netUpdate() { ntListen(); }

static unsigned uiUpdate(
    __attribute_maybe_unused__ unsigned _,
    __attribute_maybe_unused__ void* _2
) {
    SDL_CondSignal(this->uiUpdateCond);

    if (this->updateThreadCounter == NET_UPDATE_PERIOD) {
        this->updateThreadCounter = 1;
        netUpdate();
    } else
        this->updateThreadCounter++;

    return this->running ? UI_UPDATE_PERIOD : 0;
}

void lcInit() {
    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->uiUpdateCond = SDL_CreateCond();
    this->uiUpdateLock = SDL_CreateMutex();
    this->updateThreadCounter = 1;

    ntInit();
    rdInit();

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    this->uiUpdateTimerId = SDL_AddTimer(UI_UPDATE_PERIOD, &uiUpdate, NULL);
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

static void uiUpdateSynchronized(function action) {
    SDL_LockMutex(this->uiUpdateLock);
    action(NULL);
    SDL_CondWait(this->uiUpdateCond, this->uiUpdateLock);
    SDL_UnlockMutex(this->uiUpdateLock);
}

void lcLoop() {
    while (this->running) {
        if (processEvents()) {
            this->running = false;
            lcClean();
            break;
        }

        uiUpdateSynchronized((function) &rdDraw);
    }
}

void lcClean() {
    if (!this) return;

    SDL_RemoveTimer(this->uiUpdateTimerId);

    rdClean();
    ntClean();

    SDL_DestroyCond(this->uiUpdateCond);
    SDL_DestroyMutex(this->uiUpdateLock);

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
