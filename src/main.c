
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

int main(int argc, const char** argv) {
    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
    lifecycleLoop();
    lifecycleClean();

    int sgna = SDL_GetNumAllocations(); // ignore weired names, this is only to avoid calling asserted function more than once, as assert(x) is a macro, which expands to (x ? :) ... if (x) ; else __assert_fail(...)
    assert(!sgna);

    return EXIT_SUCCESS;
}
