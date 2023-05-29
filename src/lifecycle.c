
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include "render.h"
#include "defs.h"
#include "lifecycle.h"

typedef struct {
    bool running;
    pthread_cond_t uiUpdateCond;
    pthread_mutex_t uiUpdateLock;
    SDL_TimerID uiUpdateTimerId;
} this_t;

static this_t* this = NULL;

static unsigned uiUpdate(__attribute_maybe_unused__ unsigned _, __attribute_maybe_unused__ void* _2) {
    pthread_cond_signal(&(this->uiUpdateCond));
    return this->running ? UI_UPDATE_PERIOD : 0;
}

void lcInit() {
    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->uiUpdateCond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    this->uiUpdateLock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&(this->uiUpdateLock));
    action(NULL);
    pthread_cond_wait(&(this->uiUpdateCond), &(this->uiUpdateLock));
    pthread_mutex_unlock(&(this->uiUpdateLock));
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

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
