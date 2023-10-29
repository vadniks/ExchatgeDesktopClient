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

#include <sodium.h> // TODO: test only
#include <stdio.h>
#include "crypto.h"

int main(void) { // TODO: test only
    SDL_Init(0);
    assert(sodium_init() >= 0);

    byte* state = SDL_malloc(sizeof(crypto_generichash_state));
    int result = crypto_generichash_init((crypto_generichash_state*) state, NULL, 0, crypto_generichash_BYTES);
    printf("%d\n", result);
    assert(!result);

    result = crypto_generichash_update((crypto_generichash_state*) state, (byte*) "Hello", 5);
    printf("%d\n", result);
    assert(!result);

    result = crypto_generichash_update((crypto_generichash_state*) state, (byte*) " ", 1);
    printf("%d\n", result);
    assert(!result);

    result = crypto_generichash_update((crypto_generichash_state*) state, (byte*) "World!", 6);
    printf("%d\n", result);
    assert(!result);

    byte hash[crypto_generichash_BYTES] = {0};
    result = crypto_generichash_final((crypto_generichash_state*) state, hash, crypto_generichash_BYTES);
    SDL_free(state);
    printf("%d\n", result);
    assert(!result);

    printBinaryArray(hash, crypto_generichash_BYTES);

    byte* state2 = cryptoHashMultipart(NULL, NULL, 0);

    assert(!cryptoHashMultipart(state2, (byte*) "Hello", 5));
    assert(!cryptoHashMultipart(state2, (byte*) " ", 1));
    assert(!cryptoHashMultipart(state2, (byte*) "World!", 6));

    byte* hash2 = cryptoHashMultipart(state2, NULL, 0);
    printBinaryArray(hash2, CRYPTO_HASH_SIZE);

    assert(!SDL_memcmp(hash, hash2, CRYPTO_HASH_SIZE));
    SDL_free(hash2);

    SDL_Quit();

//    if (!lifecycleInit()) return EXIT_FAILURE;
//
//    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "\r\033[1;32m"
//        "_______ _     _ _______ _     _ _______ _______  ______ _______\n"
//        "|______  \\___/  |       |_____| |_____|    |    |  ____ |______\n"
//        "|______ _/   \\_ |_____  |     | |     |    |    |_____| |______\n"
//        "                   free software (GNU GPL v3)                   \033[0m"
//    );
//
//    SDL_version version;
//    SDL_GetVersion(&version);
//    assert(version.major == 2);
//
//    bool untested = false;
//    if (version.major * 1000 + version.minor * 10 + version.patch != 2265) {
//        untested = true;
//        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Tested only on 2.26.5, may work unstable on other versions"); // works on 2.28.1 but there are bugs in it
//    }
//
//    lifecycleLoop();
//    lifecycleClean();

    const int allocations = SDL_GetNumAllocations();

    if (allocations) // TODO: debug only
        fprintf(stderr, "allocations %d\n", allocations); // Cannout use SDL_Log() here 'cause it will allocate smth internally and the cleanup has been performed before

    assert(!allocations/* || untested && allocations == 1*/);
    // unknown bug occurs on 2.28.1: SDL_GetNumAllocations() returns 1 everytime only SDL_Init() was called with SDL_INIT_VIDEO constant (after SDL_Quit() was called). on 2.26.5 it returns 0 as expected

    return EXIT_SUCCESS;
}
