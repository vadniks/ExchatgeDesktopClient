
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

#include "database/database.h"
#include <sdl/SDL.h>

int main(int argc, const char** argv) {
    SDL_Init(0);
    byte akaPassword[16];
    SDL_memset(akaPassword, 'a', 16);
    assert(databaseInit(akaPassword, 16, 16, 928));
    databaseClean();
    SDL_Quit();
//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
