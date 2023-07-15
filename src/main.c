
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

#include "database/database.h"

int main(int argc, const char** argv) {
    byte* akaPassword = SDL_malloc(16); // TODO: test only
    SDL_memset(akaPassword, 0, 16);
    assert(databaseInit(akaPassword, 16));
    databaseClean();

//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
//    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
