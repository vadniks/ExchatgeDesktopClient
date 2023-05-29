
#include <stdbool.h>
#include "lifecycle.h"
#include "render.h"

typedef struct {} this_t;
static this_t* this = NULL;

void lcInit() {
    this = SDL_malloc(sizeof *this);

    rdInit();

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
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
    while (true) {
        if (processEvents()) {
            lcClean();
            break;
        }

        rdDraw();
    }
}

void lcClean() {
    if (!this) return;

    rdClean();

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
