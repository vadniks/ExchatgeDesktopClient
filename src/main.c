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

#include <stdlib.h>
#include <SDL.h>
#include <sodium.h>
#include "defs.h"
#include "crypto.h"

int main(void) {
    int n = 18, d = 16, n2;

    if (!n) n2 = 0;
    else n2 = n + 1;

//    if (n > 1) n2 = n - 1;
//    else if (n == 1) n2 = 1;
//    else n2 = 0;

    div_t r = div(n2, d);
    printf("%d %d | %d %d\n", r.quot, r.rem, /**/ d * (r.quot + (r.rem > 0 ? 1 : 0)), r.quot + 1);

    unsigned char buf[n + d];
    size_t        buf_unpadded_len = n;
    size_t        buf_padded_len;
    size_t        block_size = d;

    SDL_Init(0);
    SDL_memset(buf, 1, sizeof buf);

    if (sodium_init() < 0) exit(1);

/* round the length of the buffer to a multiple of `block_size` by appending
 * padding data and put the new, total length into `buf_padded_len` */
    printBinaryArray(buf, n);
    if (sodium_pad(&buf_padded_len, buf, buf_unpadded_len, block_size, sizeof buf) != 0) {
        /* overflow! buf[] is not large enough */
        exit(1);
    }
    printBinaryArray(buf, buf_padded_len);
    printf("%zu\n", buf_padded_len);

/* compute the original, unpadded length */
    if (sodium_unpad(&buf_unpadded_len, buf, buf_padded_len, block_size) != 0) {
        /* incorrect padding */
        exit(1);
    }
    printf("%zu\n", buf_padded_len);



    //



    unsigned size = n, newSize = 0;
    byte bytes[size];
    SDL_memset(bytes, 1, size);

    byte* new = cryptoAddPadding(&newSize, bytes, size);
    printf("\n%u\n", newSize);
    if (new) printBinaryArray(new, newSize); else printBinaryArray(bytes, newSize);

    byte* new2 = cryptoRemovePadding(&newSize, new ? new : bytes, newSize);
    SDL_free(new);

    printf(" %u\n", newSize);
    if (new2) printBinaryArray(new2, newSize); else printBinaryArray(bytes, newSize); // 1 1 1 1 1 1 1 1 1 1 1 1 1 1
    SDL_free(new2);

    SDL_Quit();

    return SDL_GetNumAllocations();
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
