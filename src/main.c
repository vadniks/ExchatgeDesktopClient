
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

#include "database/database.h"

int main(int argc, const char** argv) {
    assert(databaseInit()); // TODO: test only
    databaseClean();
//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
//    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
