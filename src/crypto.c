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

#include <SDL.h>
#include <sodium.h>
#include <assert.h>
#include "crypto.h"

typedef crypto_secretstream_xchacha20poly1305_state StreamState;

typedef unsigned long long xuint64; // even that 'long' is doubled it is still 64 bits (8 bytes) so it's just unsigned long
staticAssert(sizeof(xuint64) == sizeof(long));

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SESSIONKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_secretstream_xchacha20poly1305_KEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_generichash_BYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_secretbox_KEYBYTES == 32);
staticAssert(crypto_sign_BYTES == 64);

const unsigned CRYPTO_KEY_SIZE = crypto_secretstream_xchacha20poly1305_KEYBYTES; // 32
const unsigned CRYPTO_HEADER_SIZE = crypto_secretstream_xchacha20poly1305_HEADERBYTES; // 24
const unsigned CRYPTO_SIGNATURE_SIZE = crypto_sign_BYTES; // 64
const unsigned CRYPTO_STREAMS_STATES_SIZE = sizeof(StreamState) * 2; // 104
STATIC_CONST_UNSIGNED SERVER_SIGN_PUBLIC_KEY_SIZE = CRYPTO_KEY_SIZE;
STATIC_CONST_UNSIGNED ENCRYPTED_ADDITIONAL_BYTES_SIZE = crypto_secretstream_xchacha20poly1305_ABYTES; // 17
STATIC_CONST_UNSIGNED MAC_SIZE = crypto_secretbox_MACBYTES; // 16
STATIC_CONST_UNSIGNED NONCE_SIZE = crypto_secretbox_NONCEBYTES; // 24
static const byte TAG_INTERMEDIATE = crypto_secretstream_xchacha20poly1305_TAG_MESSAGE; // 0
__attribute_maybe_unused__ static const byte TAG_LAST = crypto_secretstream_xchacha20poly1305_TAG_FINAL; // 3

static byte serverSignPublicKey[SERVER_SIGN_PUBLIC_KEY_SIZE] = {0};
static atomic bool sodiumInitialized = false;

struct Crypto_t {
    byte serverPublicKey[CRYPTO_KEY_SIZE]; // clientPublicKey for *AsServer functions
    byte clientPublicKey[CRYPTO_KEY_SIZE]; // serverPublicKey for *AsServer functions
    byte clientSecretKey[CRYPTO_KEY_SIZE]; // serverSecretKey for *AsServer functions
    byte clientKey[CRYPTO_KEY_SIZE];
    byte serverKey[CRYPTO_KEY_SIZE];
    StreamState clientDecryptionState; // serverDecryptionState for *AsServer functions
    StreamState clientEncryptionState; // serverEncryptionState for *AsServer functions
};

Crypto* nullable cryptoInit(void) {
    if (!sodiumInitialized && sodium_init() < 0) return NULL;
    else sodiumInitialized = true;

    return (Crypto*) SDL_malloc(sizeof(Crypto));
}

void cryptoSetServerSignPublicKey(const byte* xServerSignPublicKey, unsigned serverSignPublicKeySize) {
    assert(serverSignPublicKeySize == SERVER_SIGN_PUBLIC_KEY_SIZE);
    SDL_memcpy(serverSignPublicKey, xServerSignPublicKey, SERVER_SIGN_PUBLIC_KEY_SIZE);
}

bool cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey) {
    assert(crypto && !crypto_kx_keypair(crypto->clientPublicKey, crypto->clientSecretKey));
    SDL_memcpy(crypto->serverPublicKey, serverPublicKey, CRYPTO_KEY_SIZE);

    int result = crypto_kx_client_session_keys(
        crypto->clientKey,
        crypto->serverKey,
        crypto->clientPublicKey,
        crypto->clientSecretKey,
        crypto->serverPublicKey
    );

    return !result;
}

byte* nullable cryptoInitializeCoderStreams(Crypto* crypto, const byte* serverStreamHeader) {
    assert(crypto);

    int result = crypto_secretstream_xchacha20poly1305_init_pull(
        &(crypto->clientDecryptionState), serverStreamHeader, crypto->serverKey
    );
    if (result != 0) return NULL;

    byte* clientStreamHeader = SDL_malloc(CRYPTO_HEADER_SIZE);
    result = crypto_secretstream_xchacha20poly1305_init_push(
        &(crypto->clientEncryptionState), clientStreamHeader, crypto->clientKey
    );

    if (result != 0) {
        SDL_free(clientStreamHeader);
        return NULL;
    }
    return clientStreamHeader;
}

bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize) {
    assert(unsignedSize > 0);

    const unsigned combinedSize = CRYPTO_SIGNATURE_SIZE + unsignedSize;
    byte combined[combinedSize];
    SDL_memcpy(combined, signature, CRYPTO_SIGNATURE_SIZE);
    SDL_memcpy(combined + CRYPTO_SIGNATURE_SIZE, unsignedBytes, unsignedSize);

    byte generatedUnsigned[combinedSize];
    unsigned long long generatedUnsignedSize;

    if (crypto_sign_open(
        generatedUnsigned,
        &generatedUnsignedSize,
        combined, combinedSize,
        serverSignPublicKey
    ) != 0)
        return false;

    assert(
        unsignedSize == (unsigned) generatedUnsignedSize &&
        !SDL_memcmp(unsignedBytes, generatedUnsigned, unsignedSize)
    );
    return true;
}

byte* cryptoMakeKey(const byte* passwordBuffer, unsigned size) {
    assert(passwordBuffer && size >= crypto_generichash_BYTES_MIN && size <= crypto_generichash_BYTES_MAX);

    byte* hash = SDL_malloc(CRYPTO_KEY_SIZE * sizeof(char));
    assert(!crypto_generichash(hash, CRYPTO_KEY_SIZE, passwordBuffer, size, NULL, 0));

    return hash;
}

void cryptoSetUpAutonomous(Crypto* crypto, const byte* key, const byte* nullable streamsStates) {
    assert(crypto && key);
    SDL_memcpy(&(crypto->clientKey), key, CRYPTO_KEY_SIZE);

    if (!streamsStates) {
        byte header[CRYPTO_HEADER_SIZE];
        assert(!crypto_secretstream_xchacha20poly1305_init_push(&(crypto->clientEncryptionState), header, key));
        assert(!crypto_secretstream_xchacha20poly1305_init_pull(&(crypto->clientDecryptionState), header, key));
    } else {
        const unsigned size = CRYPTO_STREAMS_STATES_SIZE / 2;
        SDL_memcpy(&(crypto->clientDecryptionState), streamsStates, size);
        SDL_memcpy(&(crypto->clientEncryptionState), streamsStates + size, size);
    }
}

byte* cryptoExportStreamsStates(const Crypto* crypto) {
    assert(crypto);
    const unsigned size = CRYPTO_STREAMS_STATES_SIZE / 2;

    byte* bytes = SDL_malloc(CRYPTO_STREAMS_STATES_SIZE);
    SDL_memcpy(bytes, &(crypto->clientDecryptionState), size);
    SDL_memcpy(bytes + size, &(crypto->clientEncryptionState), size);

    return bytes;
}

const byte* cryptoClientKey(const Crypto* crypto) {
    assert(crypto);
    return crypto->clientKey;
}

static inline byte* clientPublicKeyAsServer(Crypto* crypto) { return crypto->serverPublicKey; }
static inline byte* serverPublicKeyAsServer(Crypto* crypto) { return crypto->clientPublicKey; }
static inline byte* serverSecretKeyAsServer(Crypto* crypto) { return crypto->clientSecretKey; }

static inline StreamState* serverDecryptionStateAsServer(Crypto* crypto) { return &(crypto->clientDecryptionState); }
static inline StreamState* serverEncryptionStateAsServer(Crypto* crypto) { return &(crypto->clientEncryptionState); }

const byte* cryptoGenerateKeyPairAsServer(Crypto* crypto) {
    assert(!crypto_kx_keypair(serverPublicKeyAsServer(crypto), serverSecretKeyAsServer(crypto)));
    return crypto->clientPublicKey;
}

bool cryptoExchangeKeysAsServer(Crypto* crypto, const byte* clientPublicKey) {
    SDL_memcpy(clientPublicKeyAsServer(crypto), clientPublicKey, CRYPTO_KEY_SIZE);

    const int result = crypto_kx_server_session_keys(
        crypto->serverKey,
        crypto->clientKey,
        serverPublicKeyAsServer(crypto),
        serverSecretKeyAsServer(crypto),
        clientPublicKeyAsServer(crypto)
    );

    return !result;
}

byte* nullable cryptoCreateEncoderAsServer(Crypto* crypto) {
    byte* serverStreamHeader = SDL_malloc(CRYPTO_HEADER_SIZE);
    const int result = crypto_secretstream_xchacha20poly1305_init_push(
        serverEncryptionStateAsServer(crypto), serverStreamHeader, crypto->serverKey
    );

    if (result != 0) {
        SDL_free(serverStreamHeader);
        return NULL;
    }
    return serverStreamHeader;
}

bool cryptoCreateDecoderStreamAsServer(Crypto* crypto, const byte* clientStreamHeader) {
    const int result = crypto_secretstream_xchacha20poly1305_init_pull(
        serverDecryptionStateAsServer(crypto), clientStreamHeader, crypto->clientKey
    );
    return !result;
}

unsigned cryptoEncryptedSize(unsigned unencryptedSize)
{ return unencryptedSize + ENCRYPTED_ADDITIONAL_BYTES_SIZE; }

const byte* cryptoClientPublicKey(const Crypto* crypto) {
    assert(crypto);
    return crypto->clientPublicKey;
}

byte* nullable cryptoEncrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize, bool server) {
    assert(crypto && bytes && bytesSize > 0);

    xuint64 generatedEncryptedSize = 0;
    const xuint64 encryptedSize = cryptoEncryptedSize(bytesSize);
    byte* encrypted = SDL_malloc(encryptedSize);

    const int result = crypto_secretstream_xchacha20poly1305_push(
        server ? serverEncryptionStateAsServer(crypto) :&(crypto->clientEncryptionState),
        encrypted,
        &generatedEncryptedSize,
        bytes,
        (xuint64) bytesSize,
        NULL,
        0,
        TAG_INTERMEDIATE
    );

    if (result != 0) {
        SDL_free(encrypted);
        return NULL;
    } else {
        assert(generatedEncryptedSize == encryptedSize);
        return encrypted;
    }
}

byte* nullable cryptoDecrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize, bool server) {
    assert(crypto && bytes && bytesSize > ENCRYPTED_ADDITIONAL_BYTES_SIZE);

    xuint64 generatedDecryptedSize = 0;
    byte tag;
    const xuint64 decryptedSize = bytesSize - ENCRYPTED_ADDITIONAL_BYTES_SIZE;
    byte* decrypted = SDL_malloc(decryptedSize);

    const int result = crypto_secretstream_xchacha20poly1305_pull(
        server ? serverDecryptionStateAsServer(crypto) : &(crypto->clientDecryptionState),
        decrypted,
        &generatedDecryptedSize,
        &tag,
        bytes,
        (xuint64) bytesSize,
        NULL,
        0
    );

    if (result != 0 || tag != TAG_INTERMEDIATE) {
        SDL_free(decrypted);
        return NULL;
    } else {
        assert(generatedDecryptedSize == decryptedSize);
        return decrypted;
    }
}

void cryptoFillWithRandomBytes(byte* filled, unsigned size) {
    assert(size > 0);
    randombytes_buf(filled, size);
}

unsigned cryptoSingleEncryptedSize(unsigned unencryptedSize)
{ return MAC_SIZE + unencryptedSize + NONCE_SIZE; }

byte* nullable cryptoEncryptSingle(const byte* key, const byte* bytes, unsigned bytesSize) {
    assert(key && bytes);

    const unsigned encryptedSize = cryptoSingleEncryptedSize(bytesSize);
    byte* encrypted = SDL_calloc(encryptedSize, sizeof(char));

    byte* nonceStart = encrypted + encryptedSize - NONCE_SIZE;
    randombytes_buf(nonceStart, NONCE_SIZE);

    byte* result = crypto_secretbox_easy(
        encrypted,
        bytes,
        bytesSize,
        nonceStart,
        key
    ) == 0 ? encrypted : NULL;

    if (!result) SDL_free(encrypted);
    return result;
}

byte* nullable cryptoDecryptSingle(const byte* key, const byte* bytes, unsigned bytesSize) {
    assert(key && bytes);

    const unsigned decryptedSize = bytesSize - MAC_SIZE - NONCE_SIZE;
    byte* decrypted = SDL_calloc(decryptedSize, sizeof(char));
    const unsigned encryptedAndTagSize = bytesSize - NONCE_SIZE;

    byte* result = crypto_secretbox_open_easy(
        decrypted,
        bytes,
        encryptedAndTagSize,
        bytes + encryptedAndTagSize,
        key
    ) == 0 ? decrypted : NULL;

    if (!result) SDL_free(decrypted);
    return result;
}

void cryptoDestroy(Crypto* crypto) {
    assert(crypto);
    cryptoFillWithRandomBytes((byte*) crypto, sizeof(Crypto));
    SDL_free(crypto);
}
