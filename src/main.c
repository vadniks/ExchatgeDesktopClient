
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

int main(int argc, const char** argv) {
    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
    lifecycleLoop();
    lifecycleClean();
    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
