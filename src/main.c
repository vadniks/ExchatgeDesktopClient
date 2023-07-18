
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
        SDL_Log("a t"),
        databaseAddConversation(1, crypto);
    else
        SDL_Log("a f");
    cryptoDestroy(crypto);
    assert(databaseConversationExists(1));

    const unsigned from = 0;
    bool messageExists = databaseMessageExists(0, &from);
    SDL_Log("b %s", boolToStr(messageExists));

    char fromText[16] = {0};
    char text[10];
    SDL_memset(text, 'a', 10);
    ConversationMessage message = {0, fromText, text, 10};
    if (!messageExists)
        SDL_Log("c"),
        databaseAddMessage(&from, &message);
    assert(databaseMessageExists(0, &from));

    databaseClean();

    SDL_Quit(); // for _Log()

//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
