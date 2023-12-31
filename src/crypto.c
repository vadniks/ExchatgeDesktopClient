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
const unsigned CRYPTO_HASH_SIZE = crypto_generichash_BYTES; // 32
STATIC_CONST_UNSIGNED SERVER_SIGN_PUBLIC_KEY_SIZE = CRYPTO_KEY_SIZE;
STATIC_CONST_UNSIGNED ENCRYPTED_ADDITIONAL_BYTES_SIZE = crypto_secretstream_xchacha20poly1305_ABYTES; // 17
STATIC_CONST_UNSIGNED MAC_SIZE = crypto_secretbox_MACBYTES; // 16
STATIC_CONST_UNSIGNED NONCE_SIZE = crypto_secretbox_NONCEBYTES; // 24
static const byte TAG_INTERMEDIATE = crypto_secretstream_xchacha20poly1305_TAG_MESSAGE; // 0
__attribute_maybe_unused__ static const byte TAG_LAST = crypto_secretstream_xchacha20poly1305_TAG_FINAL; // 3

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    byte serverSignPublicKey[SERVER_SIGN_PUBLIC_KEY_SIZE];
)
#pragma clang diagnostic pop

struct CryptoKeys_t {
    byte serverPublicKey[CRYPTO_KEY_SIZE]; // clientPublicKey for *AsServer functions
    byte clientPublicKey[CRYPTO_KEY_SIZE]; // serverPublicKey for *AsServer functions
    byte clientSecretKey[CRYPTO_KEY_SIZE]; // serverSecretKey for *AsServer functions
    byte clientKey[CRYPTO_KEY_SIZE];
    byte serverKey[CRYPTO_KEY_SIZE];
};

struct CryptoCoderStreams_t {
    StreamState clientDecryptionState; // serverDecryptionState for *AsServer functions
    StreamState clientEncryptionState; // serverEncryptionState for *AsServer functions
};

struct CryptoBundle_t {
    CryptoKeys keys;
    CryptoCoderStreams coderStreams;
};

void cryptoInit(void) {
    assert(sodium_init() >= 0); // there's no sodium_destroy/clean function, allocated objects will be freed at the exit anyway
    assert(!this);

    this = SDL_malloc(sizeof *this);
    SDL_memset(this->serverSignPublicKey, 0, SERVER_SIGN_PUBLIC_KEY_SIZE);
}

CryptoKeys* cryptoKeysInit(void) { return SDL_malloc(sizeof(CryptoKeys)); }

CryptoCoderStreams* cryptoCoderStreamsInit(void) { return SDL_malloc(sizeof(CryptoCoderStreams)); }

CryptoBundle* cryptoBundleInit(const CryptoKeys* keys, const CryptoCoderStreams* coderStreams) {
    CryptoBundle* bundle = SDL_malloc(sizeof *bundle);
    bundle->keys = *keys;
    bundle->coderStreams = *coderStreams;
    return bundle;
}

CryptoKeys* cryptoBundleKeys(CryptoBundle* bundle) { return &(bundle->keys); }

CryptoCoderStreams* cryptoBundleCoderStreams(CryptoBundle* bundle) { return &(bundle->coderStreams); }

void cryptoSetServerSignPublicKey(const byte* xServerSignPublicKey, unsigned serverSignPublicKeySize) {
    assert(this);
    assert(serverSignPublicKeySize == SERVER_SIGN_PUBLIC_KEY_SIZE);
    SDL_memcpy(this->serverSignPublicKey, xServerSignPublicKey, SERVER_SIGN_PUBLIC_KEY_SIZE);
}

bool cryptoExchangeKeys(CryptoKeys* keys, const byte* serverPublicKey) {
    assert(this);
    assert(keys && !crypto_kx_keypair(keys->clientPublicKey, keys->clientSecretKey));

    SDL_memcpy(keys->serverPublicKey, serverPublicKey, CRYPTO_KEY_SIZE);

    const int result = crypto_kx_client_session_keys(
        keys->clientKey,
        keys->serverKey,
        keys->clientPublicKey,
        keys->clientSecretKey,
        keys->serverPublicKey
    );

    return !result;
}

byte* nullable cryptoInitializeCoderStreams(const CryptoKeys* keys, CryptoCoderStreams* coderStreams, const byte* serverStreamHeader) {
    assert(this);
    assert(coderStreams);

    int result = crypto_secretstream_xchacha20poly1305_init_pull(
        &(coderStreams->clientDecryptionState), serverStreamHeader, keys->serverKey
    );
    if (result != 0) return NULL;

    byte* clientStreamHeader = SDL_malloc(CRYPTO_HEADER_SIZE);
    result = crypto_secretstream_xchacha20poly1305_init_push(
        &(coderStreams->clientEncryptionState), clientStreamHeader, keys->clientKey
    );

    if (result != 0) {
        SDL_free(clientStreamHeader);
        return NULL;
    }
    return clientStreamHeader;
}

bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize) {
    assert(this);
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
        this->serverSignPublicKey
    ) != 0)
        return false;

    assert(
        unsignedSize == (unsigned) generatedUnsignedSize &&
        !SDL_memcmp(unsignedBytes, generatedUnsigned, unsignedSize)
    );
    return true;
}

byte* cryptoMakeKey(const byte* passwordBuffer, unsigned size) {
    assert(this);
    assert(passwordBuffer && size >= crypto_generichash_BYTES_MIN && size <= crypto_generichash_BYTES_MAX);

    byte* hash = SDL_malloc(CRYPTO_KEY_SIZE * sizeof(char));
    assert(!crypto_generichash(hash, CRYPTO_KEY_SIZE, passwordBuffer, size, NULL, 0));

    return hash;
}

void cryptoSetUpAutonomous(CryptoCoderStreams* coderStreams, const byte* key, const byte* nullable streamsStates) {
    assert(this);
    assert(coderStreams && key);

    if (!streamsStates) {
        byte header[CRYPTO_HEADER_SIZE];
        assert(!crypto_secretstream_xchacha20poly1305_init_push(&(coderStreams->clientEncryptionState), header, key));
        assert(!crypto_secretstream_xchacha20poly1305_init_pull(&(coderStreams->clientDecryptionState), header, key));
    } else {
        const unsigned size = CRYPTO_STREAMS_STATES_SIZE / 2;
        SDL_memcpy(&(coderStreams->clientDecryptionState), streamsStates, size);
        SDL_memcpy(&(coderStreams->clientEncryptionState), streamsStates + size, size);
    }
}

byte* cryptoExportStreamsStates(const CryptoCoderStreams* coderStreams) {
    assert(this);
    assert(coderStreams);

    const unsigned size = CRYPTO_STREAMS_STATES_SIZE / 2;

    byte* bytes = SDL_malloc(CRYPTO_STREAMS_STATES_SIZE);
    SDL_memcpy(bytes, &(coderStreams->clientDecryptionState), size);
    SDL_memcpy(bytes + size, &(coderStreams->clientEncryptionState), size);

    return bytes;
}

static inline byte* clientPublicKeyAsServer(CryptoKeys* keys) { return keys->serverPublicKey; }
static inline byte* serverPublicKeyAsServer(CryptoKeys* keys) { return keys->clientPublicKey; }
static inline byte* serverSecretKeyAsServer(CryptoKeys* keys) { return keys->clientSecretKey; }

static inline StreamState* serverDecryptionStateAsServer(CryptoCoderStreams* coderStreams) { return &(coderStreams->clientDecryptionState); }
static inline StreamState* serverEncryptionStateAsServer(CryptoCoderStreams* coderStreams) { return &(coderStreams->clientEncryptionState); }

const byte* cryptoGenerateKeyPairAsServer(CryptoKeys* keys) {
    assert(this);
    assert(!crypto_kx_keypair(serverPublicKeyAsServer(keys), serverSecretKeyAsServer(keys)));
    return keys->clientPublicKey;
}

bool cryptoExchangeKeysAsServer(CryptoKeys* keys, const byte* clientPublicKey) {
    assert(this);
    SDL_memcpy(clientPublicKeyAsServer(keys), clientPublicKey, CRYPTO_KEY_SIZE);

    const int result = crypto_kx_server_session_keys(
        keys->serverKey,
        keys->clientKey,
        serverPublicKeyAsServer(keys),
        serverSecretKeyAsServer(keys),
        clientPublicKeyAsServer(keys)
    );

    return !result;
}

byte* nullable cryptoCreateEncoderAsServer(const CryptoKeys* keys, CryptoCoderStreams* coderStreams) {
    assert(this);

    byte* serverStreamHeader = SDL_malloc(CRYPTO_HEADER_SIZE);
    const int result = crypto_secretstream_xchacha20poly1305_init_push(
        serverEncryptionStateAsServer(coderStreams), serverStreamHeader, keys->serverKey
    );

    if (result != 0) {
        SDL_free(serverStreamHeader);
        return NULL;
    }
    return serverStreamHeader;
}

bool cryptoCreateDecoderStreamAsServer(const CryptoKeys* keys, CryptoCoderStreams* coderStreams, const byte* clientStreamHeader) {
    assert(this);

    const int result = crypto_secretstream_xchacha20poly1305_init_pull(
        serverDecryptionStateAsServer(coderStreams), clientStreamHeader, keys->clientKey
    );

    return !result;
}

unsigned cryptoEncryptedSize(unsigned unencryptedSize)
{ return unencryptedSize + ENCRYPTED_ADDITIONAL_BYTES_SIZE; }

const byte* cryptoClientPublicKey(const CryptoKeys* keys) {
    assert(keys);
    return keys->clientPublicKey;
}

byte* nullable cryptoEncrypt(CryptoCoderStreams* coderStreams, const byte* bytes, unsigned bytesSize, bool server) {
    assert(this);
    assert(coderStreams && bytes && bytesSize > 0);

    unsigned long long generatedEncryptedSize = 0; // same as unsigned long
    const unsigned long encryptedSize = cryptoEncryptedSize(bytesSize);
    byte* encrypted = SDL_malloc(encryptedSize);

    const int result = crypto_secretstream_xchacha20poly1305_push(
        server ? serverEncryptionStateAsServer(coderStreams) : &(coderStreams->clientEncryptionState),
        encrypted,
        &generatedEncryptedSize,
        bytes,
        (unsigned long) bytesSize,
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

byte* nullable cryptoDecrypt(CryptoCoderStreams* coderStreams, const byte* bytes, unsigned bytesSize, bool server) {
    assert(this);
    assert(coderStreams && bytes && bytesSize > ENCRYPTED_ADDITIONAL_BYTES_SIZE);

    unsigned long long generatedDecryptedSize = 0; // same as unsigned long
    byte tag;
    const unsigned long decryptedSize = bytesSize - ENCRYPTED_ADDITIONAL_BYTES_SIZE;
    byte* decrypted = SDL_malloc(decryptedSize);

    const int result = crypto_secretstream_xchacha20poly1305_pull(
        server ? serverDecryptionStateAsServer(coderStreams) : &(coderStreams->clientDecryptionState),
        decrypted,
        &generatedDecryptedSize,
        &tag,
        bytes,
        (unsigned long) bytesSize,
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
    assert(this);
    assert(size > 0);
    randombytes_buf(filled, size);
}

unsigned cryptoSingleEncryptedSize(unsigned unencryptedSize)
{ return MAC_SIZE + unencryptedSize + NONCE_SIZE; }

byte* nullable cryptoEncryptSingle(const byte* key, const byte* bytes, unsigned bytesSize) {
    assert(this);
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
    assert(this);
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

char* cryptoBase64Encode(const byte* bytes, unsigned bytesSize) {
    assert(this);
    assert(bytesSize);

    const unsigned encodedSize = sodium_base64_encoded_len(bytesSize, sodium_base64_VARIANT_URLSAFE);
    assert(encodedSize);

    char* encoded = SDL_malloc(encodedSize);
    assert(sodium_bin2base64(encoded, encodedSize, bytes, bytesSize, sodium_base64_VARIANT_URLSAFE));
    assert(!encoded[encodedSize - 1]);

    return encoded;
}

byte* nullable cryptoBase64Decode(const char* encoded, unsigned encodedSize, unsigned* xDecodedSize) {
    assert(this);

    const unsigned decodedSize = encodedSize / 4 * 3;
    if (!encodedSize || !decodedSize) return NULL;

    byte* decoded = SDL_malloc(decodedSize);
    unsigned long actualDecodedSize = 0;

    if (sodium_base642bin(
        decoded,
        decodedSize,
        encoded,
        encodedSize,
        "",
        &actualDecodedSize,
        NULL,
        sodium_base64_VARIANT_URLSAFE
    ) != 0 || !actualDecodedSize) {
        SDL_free(decoded);
        return NULL;
    }

    decoded = SDL_realloc(decoded, actualDecodedSize);
    *xDecodedSize = (unsigned) actualDecodedSize;
    return decoded;
}

void* nullable cryptoHashMultipart(void* nullable previous, const byte* nullable bytes, unsigned size) {
    assert(this);

    if (!previous && !bytes) {
        previous = SDL_malloc(sizeof(crypto_generichash_state));
        assert(!crypto_generichash_init((crypto_generichash_state*) previous, NULL, 0, CRYPTO_HASH_SIZE));
        return previous;
    } else if (previous && bytes) {
        assert(!crypto_generichash_update((crypto_generichash_state*) previous, bytes, size));
        return NULL;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"

    } else if (previous && !bytes) {

#pragma clang diagnostic pop

        byte* hash = SDL_malloc(CRYPTO_HASH_SIZE);
        assert(!crypto_generichash_final((crypto_generichash_state*) previous, hash, CRYPTO_HASH_SIZE));
        SDL_free(previous);
        return hash;
    } else
        assert(false);
}

static void randomiseAndFree(void* object, unsigned size) {
    assert(this);
    assert(object);

    cryptoFillWithRandomBytes((byte*) object, size);
    SDL_free(object);
}

void cryptoKeysDestroy(CryptoKeys* keys) { randomiseAndFree(keys, sizeof *keys); }

void cryptoCoderStreamsDestroy(CryptoCoderStreams* coderStreams) { randomiseAndFree(coderStreams, sizeof *coderStreams); }

void cryptoBundleDestroy(CryptoBundle* bundle) { randomiseAndFree(bundle, sizeof *bundle); }

void cryptoClean(void) {
    assert(this);
    SDL_free(this);
    this = NULL;
}
