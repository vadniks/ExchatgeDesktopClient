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

#pragma ide diagnostic ignored "misc-no-recursion"

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

void testCrypto_singleCrypt(void) {
    const int allocations = SDL_GetNumAllocations();

    byte key[CRYPTO_KEY_SIZE];
    cryptoFillWithRandomBytes(key, CRYPTO_KEY_SIZE);

    {
        const unsigned size = 10;
        byte original[size];
        cryptoFillWithRandomBytes(original, size);

        byte* encrypted = cryptoEncryptSingle(key, original, size);
        assert(encrypted);

        byte* decrypted = cryptoDecryptSingle(key, encrypted, cryptoSingleEncryptedSize(size));
        assert(decrypted);

        assert(!SDL_memcmp(original, decrypted, size));

        SDL_free(encrypted);
        SDL_free(decrypted);
    }

    assert(allocations == SDL_GetNumAllocations());
}

void testCrypto_hash(void) {
    const int allocations = SDL_GetNumAllocations();

    const unsigned size = 1 << 10, slice = size / 8;
    byte bytes[size];
    cryptoFillWithRandomBytes(bytes, size);

    void* state = cryptoHashMultipart(NULL, NULL, 0);
    assert(state);

    for (unsigned i = 0; i < size; i += slice)
        assert(!cryptoHashMultipart(state, bytes + i, slice));

    byte* hash = cryptoHashMultipart(state, NULL, 0);
    assert(hash);

    SDL_free(hash);

    assert(allocations == SDL_GetNumAllocations());
}

void testCrypto_padding(bool first) {
    const int allocations = SDL_GetNumAllocations();

    const unsigned size = first ? 10 : CRYPTO_PADDING_BLOCK_SIZE;
    byte original[size];
    SDL_memset(original, 0x80, size);

    unsigned paddedSize;
    byte* padded = cryptoAddPadding(&paddedSize, original, size);
    assert(padded);
    assert(paddedSize % CRYPTO_PADDING_BLOCK_SIZE == 0 && paddedSize > size);

    unsigned unpaddedSize;
    byte* unpadded = cryptoRemovePadding(&unpaddedSize, padded, paddedSize);
    assert(unpadded);
    assert(unpaddedSize == size);

    assert(!SDL_memcmp(original, unpadded, size));

    SDL_free(padded);
    SDL_free(unpadded);

    assert(allocations == SDL_GetNumAllocations());
    first ? testCrypto_padding(false) : STUB;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
void testCrypto_coderStreamsSerialization(void) {
    const int allocations = SDL_GetNumAllocations();

    const CryptoCoderStreams* original = (CryptoCoderStreams*) (byte[]) {
        0xcb, 0xaf, 0x03, 0x4e, 0xbe, 0xfc, 0x5b, 0x5f, 0x1c, 0xb0, 0x2f, 0x73, 0x45, 0x0f, 0x29, 0x17,
        0x43, 0xb7, 0x84, 0xc2, 0xcf, 0x4d, 0x59, 0x22, 0x57, 0x78, 0xb8, 0x7c, 0xf4, 0x9b, 0x9f, 0xe2,
        0x01, 0x00, 0x00, 0x00, 0x9d, 0x1a, 0x72, 0xf4, 0xdb, 0x67, 0xec, 0x0c, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xcb, 0xaf, 0x03, 0x4e, 0xbe, 0xfc, 0x5b, 0x5f, 0x1c, 0xb0, 0x2f, 0x73,
        0x45, 0x0f, 0x29, 0x17, 0x43, 0xb7, 0x84, 0xc2, 0xcf, 0x4d, 0x59, 0x22, 0x57, 0x78, 0xb8, 0x7c,
        0xf4, 0x9b, 0x9f, 0xe2, 0x01, 0x00, 0x00, 0x00, 0x9d, 0x1a, 0x72, 0xf4, 0xdb, 0x67, 0xec, 0x0c,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    byte* serialized = cryptoExportStreamsStates(original);

    CryptoCoderStreams* deserialized = SDL_malloc(CRYPTO_STREAMS_STATES_SIZE);
    cryptoSetUpAutonomous(deserialized, (byte[0]) {}, serialized);
    SDL_free(serialized);

    assert(!SDL_memcmp(original, deserialized, CRYPTO_STREAMS_STATES_SIZE));
    SDL_free(deserialized);

    assert(allocations == SDL_GetNumAllocations());
}
#pragma clang diagnostic pop

void testCrypto_base64(void) {
    const int allocations = SDL_GetNumAllocations();

    const unsigned size = 10;
    byte original[size];
    cryptoFillWithRandomBytes(original, size);

    char* encoded = cryptoBase64Encode(original, size);
    unsigned encodedSize = SDL_strlen(encoded), decodedSize;
    assert(encodedSize > size);

    byte* decoded = cryptoBase64Decode(encoded, encodedSize, &decodedSize);
    SDL_free(encoded);

    assert(size == decodedSize);
    assert(!SDL_memcmp(original, decoded, size));
    SDL_free(decoded);

    assert(allocations == SDL_GetNumAllocations());
}
