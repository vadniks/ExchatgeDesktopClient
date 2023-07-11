
#include <sdl/SDL_stdinc.h>
#include "user.h"

List* userInitList(void) { return listInit((ListDeallocator) &userDestroy); }

User* userCreate(
    unsigned id,
    const char* name,
    unsigned nameSize,
    bool conversationExists,
    bool online
) {
    User* user = SDL_malloc(sizeof *user);
    user->id = id;
    user->name = SDL_calloc(nameSize, sizeof(char));
    SDL_memcpy(user->name, name, nameSize);
    user->conversationExists = conversationExists;
    user->online = online;
    return user;
}

void userDestroy(User* user) {
    SDL_free(user->name);
    SDL_free(user);
}
