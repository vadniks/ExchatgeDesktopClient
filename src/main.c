
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
    assert(databaseInit(akaPassword, 16, 16, 928));

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

    byte text[10];
    SDL_memset(text, 'a', 10);
    DatabaseMessage* message = databaseMessageCreate(0, &from, text, 10);
    if (!messageExists)
        SDL_Log("c"),
        databaseAddMessage(message);
    databaseMessageDestroy(message);
    assert(databaseMessageExists(0, &from));

    List* messages = databaseGetMessages(0);
    assert(messages);
    for (unsigned i = 0; i < listSize(messages); message = listGet(messages, i++)) SDL_Log("%lu %ld %s %u",
        databaseMessageTimestamp(message),
        databaseMessageFrom(message) ? (long) *databaseMessageFrom(message) : -1l,
        databaseMessageText(message),
        databaseMessageSize(message)
    );
    listDestroy(messages);

    databaseClean();

    SDL_Quit(); // for _Log()

//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
