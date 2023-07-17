
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

#include <sdl/SDL.h>
#include "database/database.h"

int main(int argc, const char** argv) {
    SDL_Init(0); // for _Log()

    byte* akaPassword = SDL_malloc(16); // TODO: test only
    SDL_memset(akaPassword, 0, 16);
    assert(databaseInit(akaPassword, 16, 16));

    Crypto* crypto = cryptoInit();
    if (!databaseConversationExists(1))
        SDL_Log("t"),
        databaseAddConversation(1, crypto);
    else
        SDL_Log("f");
    cryptoDestroy(crypto);
    assert(databaseConversationExists(1));

    databaseClean();

//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();

    SDL_Quit(); // for _Log()

    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
