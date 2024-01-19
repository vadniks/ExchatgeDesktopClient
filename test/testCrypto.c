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

#include <assert.h>
#include <SDL.h>
#include <alloca.h>
#include "../src/crypto.h"
#include "testCrypto.h"

void testCrypto_start(void) { cryptoInit(); }
void testCrypto_stop(void) { cryptoClean(); }

void testCrypto_keyExchange(void) {
    const int allocations = SDL_GetNumAllocations();

    CryptoKeys* serverKeys = cryptoKeysInit();
    const byte* serverPublicKey = cryptoGenerateKeyPairAsServer(serverKeys);

    CryptoKeys* clientKeys = cryptoKeysInit();
    assert(cryptoExchangeKeys(clientKeys, serverPublicKey));

    assert(cryptoExchangeKeysAsServer(serverKeys, cryptoClientPublicKey(clientKeys)));

    const void* sharedServerKeys = (void*) serverKeys + CRYPTO_KEY_SIZE * 3;
    const void* sharedClientKeys = (void*) clientKeys + CRYPTO_KEY_SIZE * 3;
    const unsigned sharedKeysSize = CRYPTO_KEY_SIZE * 2;

    assert(!SDL_memcmp(sharedServerKeys, sharedClientKeys, sharedKeysSize));
    assert(!SDL_memcmp(exposedTestCrypto_sharedDecryptionKey(clientKeys), exposedTestCrypto_sharedDecryptionKey(serverKeys), CRYPTO_KEY_SIZE));
    assert(!SDL_memcmp(exposedTestCrypto_sharedEncryptionKey(clientKeys), exposedTestCrypto_sharedEncryptionKey(serverKeys), CRYPTO_KEY_SIZE));
    assert(!SDL_memcmp(sharedServerKeys, exposedTestCrypto_sharedEncryptionKey(clientKeys), CRYPTO_KEY_SIZE));
    assert(!SDL_memcmp(sharedClientKeys + CRYPTO_KEY_SIZE, exposedTestCrypto_sharedDecryptionKey(serverKeys), CRYPTO_KEY_SIZE));

    cryptoKeysDestroy(serverKeys);
    cryptoKeysDestroy(clientKeys);

    assert(allocations == SDL_GetNumAllocations());
}

void testCrypto_signature(void) {
    const int allocations = SDL_GetNumAllocations();

    byte* publicSignKey = SDL_malloc(CRYPTO_KEY_SIZE);
    byte* secretSignKey = SDL_malloc(EXPOSED_TEST_CRYPTO_SIGN_SECRET_KEY_SIZE);
    exposedTestCrypto_makeSignKeys(publicSignKey, secretSignKey);

    cryptoSetServerSignPublicKey(publicSignKey, CRYPTO_KEY_SIZE);
    SDL_free(publicSignKey);

    const unsigned size = 10;
    byte bytes[size];
    cryptoFillWithRandomBytes(bytes, size);

    byte* xSigned = exposedTestCrypto_sign(bytes, size, secretSignKey);
    assert(xSigned);
    SDL_free(secretSignKey);

    assert(cryptoCheckServerSignedBytes(xSigned, xSigned + CRYPTO_SIGNATURE_SIZE, size));
    SDL_free(xSigned);

    assert(allocations == SDL_GetNumAllocations());
}

void testCrypto_streamCrypt(void) {
    const int allocations = SDL_GetNumAllocations();

    byte key[CRYPTO_KEY_SIZE];
    cryptoFillWithRandomBytes(key, CRYPTO_KEY_SIZE);

    CryptoKeys* keys = alloca(CRYPTO_KEY_SIZE * 5);
    SDL_memcpy((void*) keys + CRYPTO_KEY_SIZE * 3, key, CRYPTO_KEY_SIZE);
    SDL_memcpy((void*) keys + CRYPTO_KEY_SIZE * 4, key, CRYPTO_KEY_SIZE);

    CryptoCoderStreams* streams1 = cryptoCoderStreamsInit();
    byte* header = cryptoCreateEncoderAsServer(keys, streams1);
    assert(header);
    assert(cryptoCreateDecoderStreamAsServer(keys, streams1, header));
    SDL_free(header);

    {
        const unsigned size = 10;
        byte original[size];
        cryptoFillWithRandomBytes(original, size);

        byte* encrypted = cryptoEncrypt(streams1, original, size, false);
        assert(encrypted);

        byte* decrypted = cryptoDecrypt(streams1, encrypted, cryptoEncryptedSize(size), false);
        assert(decrypted);
        SDL_free(encrypted);

        assert(!SDL_memcmp(original, decrypted, size));
        SDL_free(decrypted);
    }

    cryptoCoderStreamsDestroy(streams1);

    assert(allocations == SDL_GetNumAllocations());
}
