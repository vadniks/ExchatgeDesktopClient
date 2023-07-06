
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include <assert.h>
#include "crypto.h"

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SESSIONKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_secretstream_xchacha20poly1305_KEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_secretbox_KEYBYTES == 32);
staticAssert(crypto_sign_BYTES == 64);

const unsigned CRYPTO_KEY_SIZE = crypto_secretstream_xchacha20poly1305_KEYBYTES; // 32
const unsigned CRYPTO_HEADER_SIZE = crypto_secretstream_xchacha20poly1305_HEADERBYTES;
const unsigned CRYPTO_SIGNATURE_SIZE = crypto_sign_BYTES; // 64
STATIC_CONST_UNSIGNED ENCRYPTED_ADDITIONAL_BYTES_SIZE = crypto_secretstream_xchacha20poly1305_ABYTES; // 17
STATIC_CONST_UNSIGNED SERVER_SIGN_PUBLIC_KEY_SIZE = CRYPTO_KEY_SIZE;
static const byte INTERMEDIATE_TAG = crypto_secretstream_xchacha20poly1305_TAG_MESSAGE; // 0
static const byte END_TAG = crypto_secretstream_xchacha20poly1305_TAG_FINAL; // 3

static const byte serverSignPublicKey[SERVER_SIGN_PUBLIC_KEY_SIZE] = {
    255, 23, 21, 243, 148, 177, 186, 0, 73, 34, 173, 130, 234, 251, 83, 130,
    138, 54, 215, 5, 170, 139, 175, 148, 71, 215, 74, 172, 27, 225, 26, 249
};

struct Crypto_t {
    byte serverPublicKey[CRYPTO_KEY_SIZE];
    byte clientPublicKey[CRYPTO_KEY_SIZE];
    byte clientSecretKey[CRYPTO_KEY_SIZE];
    crypto_secretstream_xchacha20poly1305_state state;
};

Crypto* cryptoInit(void) {
    if (sodium_init() < 0) return NULL;

    Crypto* crypto = SDL_malloc(sizeof *crypto);
    SDL_memset(crypto->serverPublicKey, 0, CRYPTO_KEY_SIZE);
    SDL_memset(crypto->clientPublicKey, 0, CRYPTO_KEY_SIZE);
    SDL_memset(crypto->clientSecretKey, 0, CRYPTO_KEY_SIZE);

    return crypto;
}

byte* nullable cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey) {
    assert(crypto);
    crypto_kx_keypair(crypto->clientPublicKey, crypto->clientSecretKey);
    SDL_memcpy(crypto->serverPublicKey, serverPublicKey, CRYPTO_KEY_SIZE);

    byte encryptionKey[CRYPTO_KEY_SIZE];
    byte* header = SDL_calloc(CRYPTO_HEADER_SIZE, sizeof(char));

    int result = crypto_kx_client_session_keys(
        encryptionKey,
        NULL,
        crypto->clientPublicKey,
        crypto->clientSecretKey,
        crypto->serverPublicKey
    );

    if (result != 0) {
        SDL_free(header);
        return NULL;
    }

    result = crypto_secretstream_xchacha20poly1305_init_push(&(crypto->state), header, encryptionKey);
    if (result != 0) SDL_free(header);

    return !result ? header : NULL;
}

unsigned cryptoEncryptedSize(unsigned unencryptedSize)
{ return unencryptedSize + ENCRYPTED_ADDITIONAL_BYTES_SIZE; }

const byte* cryptoClientPublicKey(const Crypto* crypto) {
    assert(crypto);
    return crypto->clientPublicKey;
}

byte* nullable cryptoEncrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize) {
    assert(crypto && bytes);

    const unsigned encryptedSize = cryptoEncryptedSize(bytesSize);
    unsigned long long generatedEncryptedSize = 0;
    byte* encrypted = SDL_calloc(encryptedSize, sizeof(char));

    byte* result = !crypto_secretstream_xchacha20poly1305_push(
        &(crypto->state),
        encrypted,
        &generatedEncryptedSize,
        bytes,
        bytesSize,
        NULL,
        0,
        INTERMEDIATE_TAG
    ) ? encrypted : NULL;

    if (result) assert((unsigned) generatedEncryptedSize == encryptedSize);
    else SDL_free(encrypted);

    return result;
}

byte* nullable cryptoDecrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize) {
    assert(crypto && bytes);

    const unsigned decryptedSize = bytesSize - ENCRYPTED_ADDITIONAL_BYTES_SIZE;
    unsigned long long generatedDecryptedSize = 0;
    byte tag;
    byte* decrypted = SDL_calloc(decryptedSize, sizeof(char));

    byte* result = !crypto_secretstream_xchacha20poly1305_pull(
        &(crypto->state),
        decrypted,
        &generatedDecryptedSize,
        &tag,
        bytes,
        bytesSize,
        NULL,
        0
    ) ? decrypted : NULL;

    if (result) assert((unsigned) generatedDecryptedSize == decryptedSize);
    else SDL_free(decrypted);

    return result;
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

void cryptoFillWithRandomBytes(byte* filled, unsigned size) {
    assert(filled && size > 0);
    randombytes_buf(filled, size);
}

void cryptoDestroy(Crypto* crypto) {
    assert(crypto);
    SDL_free(crypto);
}
