
#include <assert.h>
#include <SDL.h>
#include "collections.h"

int main(void) {
    assert(!SDL_Init(0));

    testCollections_list();

    SDL_Quit();
    return EXIT_SUCCESS;
}
