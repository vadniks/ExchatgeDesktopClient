/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)
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

#define DEVELOPMENT_MODE 0 // TODO: debug

#if DEVELOPMENT_MODE == 1 // Designing & testing new feature

#include <assert.h>
#include <SDL.h>
#include <sodium.h>
#include "defs.h"
#include "crypto.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-auto-type"
int main(void) {
    SDL_Init(0);
    assert(sodium_init() >= 0);

    {
        auto_type original = (const byte*) "Hello World!";
        auto_type originalSize = SDL_strlen((char*) original);
        auto_type encryptedSize = crypto_box_SEALBYTES + originalSize;

        const auto_type pkSize = crypto_box_PUBLICKEYBYTES;
        staticAssert(pkSize == crypto_box_SECRETKEYBYTES);

        byte publicKey[pkSize], secretKey[pkSize];
        assert(!crypto_box_keypair(publicKey, secretKey));

        byte encrypted[encryptedSize];
        assert(!crypto_box_seal(encrypted, original, originalSize, publicKey));

        byte decrypted[originalSize];
        assert(!crypto_box_seal_open(decrypted, encrypted, encryptedSize, publicKey, secretKey));
        assert(!SDL_memcmp(original, decrypted, originalSize));
    }

    {
        const unsigned keySize = crypto_secretbox_KEYBYTES;
        staticAssert(keySize == crypto_kdf_KEYBYTES);
        byte masterKey[keySize], key0[keySize], key1[keySize], key2[keySize];
        crypto_kdf_keygen(masterKey);

        assert(!crypto_kdf_blake2b_derive_from_key(key0, keySize, 0, "0", masterKey));
        assert(!crypto_kdf_blake2b_derive_from_key(key1, keySize, 1, "1", masterKey));
        assert(!crypto_kdf_blake2b_derive_from_key(key2, keySize, 2, "2", masterKey));

        printBinaryArray(masterKey, keySize);
        printBinaryArray(key0, keySize);
        printBinaryArray(key1, keySize);
        printBinaryArray(key2, keySize);

        const unsigned nonceSize = crypto_secretbox_NONCEBYTES;
        byte nonce0[nonceSize], nonce1[nonceSize], nonce2[nonceSize];
        randombytes_buf(nonce0, nonceSize);

        SDL_memcpy(nonce1, nonce0, nonceSize);
        sodium_increment(nonce1, nonceSize);

        SDL_memcpy(nonce2, nonce1, nonceSize);
        sodium_increment(nonce2, nonceSize);

        printBinaryArray(nonce0, nonceSize);
        printBinaryArray(nonce1, nonceSize);
        printBinaryArray(nonce2, nonceSize);

        const unsigned messageSize = 10;
        byte message0[messageSize], message1[messageSize], message2[messageSize];
        randombytes_buf(message0, messageSize);
        randombytes_buf(message1, messageSize);
        randombytes_buf(message2, messageSize);

        const unsigned encryptedSize = crypto_secretbox_MACBYTES + messageSize;
        byte encrypted[encryptedSize];

        assert(!crypto_secretbox_easy(encrypted, message0, messageSize, nonce0, key0));
        sodium_increment(nonce0, nonceSize);

//        assert(!crypto_secretbox_easy(encrypted, message1, messageSize, nonce1, key1));
//        sodium_increment(nonce1, nonceSize);
//
//        assert(!crypto_secretbox_easy(encrypted, message2, messageSize, nonce2, key2));
//        sodium_increment(nonce2, nonceSize);

        byte decrypted[messageSize];

        sodium_sub(nonce0, (byte[keySize]) {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, keySize); // *
        assert(!crypto_secretbox_open_easy(decrypted, encrypted, encryptedSize, nonce0, key0));
        assert(!SDL_memcmp(message0, decrypted, messageSize));
    }

    SDL_Quit();
    return 0;
}
#pragma clang diagnostic pop

#else

#include <stdlib.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include "lifecycle.h"

int main(void) {
    if (!lifecycleInit()) return EXIT_FAILURE;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "\r\033[1;32m" // Figlet
        "           _______ _     _ _______ _     _ _______ _______  ______ _______            \n"
        "           |______  \\___/  |       |_____| |_____|    |    |  ____ |______           \n"
        "           |______ _/   \\_ |_____  |     | |     |    |    |_____| |______           \n"
        "Exchatge (Client) Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)\n"
        "                   This program comes with ABSOLUTELY NO WARRANTY;                    \n"
        "This is free software, and you are welcome to redistribute it under certain conditions\n"
        "\033[0m"
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
