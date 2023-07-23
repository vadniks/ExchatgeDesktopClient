/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
