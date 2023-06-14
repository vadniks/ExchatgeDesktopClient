
#pragma clang diagnostic ignored "-Wgnu-folding-constant"

#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include <assert.h>
#include "crypto.h"

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SESSIONKEYBYTES == crypto_secretbox_KEYBYTES);

const unsigned CRYPTO_KEY_SIZE = crypto_secretbox_KEYBYTES; // 32
static const unsigned MAC_SIZE = crypto_secretbox_MACBYTES; // 16
static const unsigned NONCE_SIZE = crypto_secretbox_NONCEBYTES; // 24

struct Crypto_t {
    byte serverPublicKey[CRYPTO_KEY_SIZE];
    byte clientPublicKey[CRYPTO_KEY_SIZE];
    byte clientSecretKey[CRYPTO_KEY_SIZE];
    byte encryptionKey[CRYPTO_KEY_SIZE];
};

Crypto* cryptoInit() {
    if (sodium_init() < 0) return NULL;

    Crypto* crypto = SDL_malloc(sizeof *crypto);
    SDL_memset(crypto->serverPublicKey, 0, CRYPTO_KEY_SIZE);
    SDL_memset(crypto->clientPublicKey, 0, CRYPTO_KEY_SIZE);
    SDL_memset(crypto->clientSecretKey, 0, CRYPTO_KEY_SIZE);
    SDL_memset(crypto->encryptionKey, 0, CRYPTO_KEY_SIZE);

    return crypto;
}

bool cryptoExchangeKeys(Crypto* crypto) {
    assert(crypto);
    crypto_kx_keypair(crypto->clientPublicKey, crypto->clientSecretKey);

    int result = crypto_kx_client_session_keys(
        crypto->encryptionKey,
        NULL,
        crypto->clientPublicKey,
        crypto->clientSecretKey,
        crypto->serverPublicKey
    );

    return !result;
}

void cryptoSetServerPublicKey(Crypto* crypto, const byte* key) {
    assert(crypto);
    SDL_memcpy(crypto->serverPublicKey, key, CRYPTO_KEY_SIZE);
}

void cryptoSetEncryptionKey(Crypto* crypto, const byte* key) {
    assert(crypto && key);
    SDL_memcpy(crypto->encryptionKey, key, CRYPTO_KEY_SIZE);
}

unsigned cryptoEncryptedSize(unsigned unencryptedSize)
{ return MAC_SIZE + unencryptedSize + NONCE_SIZE; }

byte* cryptoClientPublicKey(Crypto* crypto) {
    assert(crypto);
    return crypto->clientPublicKey;
}

byte* nullable cryptoEncrypt(const Crypto* crypto, const byte* bytes, unsigned bytesSize) {
    assert(crypto && bytes);

    const unsigned encryptedSize = cryptoEncryptedSize(bytesSize);
    byte* encrypted = SDL_calloc(encryptedSize, sizeof(char));

    byte* nonceStart = encrypted + encryptedSize - NONCE_SIZE;
    randombytes_buf(nonceStart, NONCE_SIZE);

    byte* result = crypto_secretbox_easy(
        encrypted,
        bytes,
        bytesSize,
        nonceStart,
        crypto->encryptionKey
    ) == 0 ? encrypted : NULL;

    if (!result) SDL_free(encrypted);
    return result;
}

byte* nullable cryptoDecrypt(const Crypto* crypto, const byte* bytes, unsigned bytesSize) {
    assert(crypto && bytes);

    const unsigned decryptedSize = bytesSize - MAC_SIZE - NONCE_SIZE;
    byte* decrypted = SDL_calloc(decryptedSize, sizeof(char));
    const unsigned encryptedAndTagSize = bytesSize - NONCE_SIZE;

    byte* result = crypto_secretbox_open_easy(
        decrypted,
        bytes,
        encryptedAndTagSize,
        bytes + encryptedAndTagSize,
        crypto->encryptionKey
    ) == 0 ? decrypted : NULL;

    if (!result) SDL_free(decrypted);
    return result;
}

void cryptoDestroy(Crypto* crypto) {
    assert(crypto);
    SDL_free(crypto);
}
