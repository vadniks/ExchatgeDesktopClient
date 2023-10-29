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

#include <stdlib.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include "lifecycle.h"

int main(void) {
    if (!lifecycleInit()) return EXIT_FAILURE;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "\r\033[1;32m"
        "_______ _     _ _______ _     _ _______ _______  ______ _______\n"
        "|______  \\___/  |       |_____| |_____|    |    |  ____ |______\n"
        "|______ _/   \\_ |_____  |     | |     |    |    |_____| |______\n"
        "                   free software (GNU GPL v3)                   \033[0m"
    );

    SDL_version version;
    SDL_GetVersion(&version);
    assert(version.major == 2);

    bool untested = false;
    if (version.major * 1000 + version.minor * 10 + version.patch != 2265) {
        untested = true;
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Tested only on 2.26.5, may work unstable on other versions"); // works on 2.28.1 but there are bugs in it
    }

    lifecycleLoop();
    lifecycleClean();

    const int allocations = SDL_GetNumAllocations();

    if (allocations) // TODO: debug only
        fprintf(stderr, "allocations %d\n", allocations); // Cannout use SDL_Log() here 'cause it will allocate smth internally and the cleanup has been performed before

    assert(!allocations || untested && allocations == 1);
    // unknown bug occurs on 2.28.1: SDL_GetNumAllocations() returns 1 everytime only SDL_Init() was called with SDL_INIT_VIDEO constant (after SDL_Quit() was called). on 2.26.5 it returns 0 as expected

    return EXIT_SUCCESS;
}
