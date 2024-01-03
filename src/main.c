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

#define DEVELOPMENT_MODE 1 // TODO: debug

#if DEVELOPMENT_MODE == 1 // Designing & testing new feature

#include <SDL.h>
#include <sodium.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include "defs.h"

int main(void) {
    assert(!SDL_Init(0));
    assert(sodium_init() >= 0);

    { // broadcast from admin to all other users - only admin can write, others can only read
        const int size = 5, encryptedSize = crypto_box_SEALBYTES + size;

        staticAssert(crypto_box_PUBLICKEYBYTES == crypto_box_SECRETKEYBYTES);
        byte recipientPublicKey[crypto_box_PUBLICKEYBYTES]; // each client except admin
        byte recipientSecretKey[crypto_box_SECRETKEYBYTES];
        assert(!crypto_box_keypair(recipientPublicKey, recipientSecretKey));

        byte text[size], encrypted[encryptedSize], decrypted[size];
        srand(time(NULL));

        for (unsigned i = 0; i < 5; i++) {
            for (char j = 0; j < size; text[j++] = rand() % ('z' + 1 - 'a') + 'a');
            SDL_Log("1 |%.*s|", size, text);

            assert(!crypto_box_seal(encrypted, text, size, recipientPublicKey)); // admin
            assert(!crypto_box_seal_open(decrypted, encrypted, encryptedSize, recipientPublicKey, recipientSecretKey));

            SDL_Log("2 |%.*s|", size, decrypted);
            SDL_Log("");
        }
    }

    SDL_Quit();
    return 0;
}

#else

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
        fprintf(stderr, "allocations %d\n", allocations); // Cannot use SDL_Log() here 'cause it will allocate smth internally and the cleanup has been performed before

    assert(!allocations || untested && allocations == 1);
    // unknown bug occurs on 2.28.1: SDL_GetNumAllocations() returns 1 everytime only SDL_Init() was called with SDL_INIT_VIDEO constant (after SDL_Quit() was called). on 2.26.5 it returns 0 as expected

    return EXIT_SUCCESS;
}

#endif
