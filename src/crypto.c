
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include <assert.h>
#include "crypto.h"

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SESSIONKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_secretstream_xchacha20poly1305_KEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_secretbox_KEYBYTES == 32);
staticAssert(crypto_sign_BYTES == 64);

const unsigned CRYPTO_KEY_SIZE = crypto_secretstream_xchacha20poly1305_KEYBYTES; // 32
const unsigned CRYPTO_HEADER_SIZE = crypto_secretstream_xchacha20poly1305_HEADERBYTES; // 24
const unsigned CRYPTO_SIGNATURE_SIZE = crypto_sign_BYTES; // 64
STATIC_CONST_UNSIGNED SERVER_SIGN_PUBLIC_KEY_SIZE = CRYPTO_KEY_SIZE;
STATIC_CONST_UNSIGNED ENCRYPTED_ADDITIONAL_BYTES_SIZE = crypto_secretstream_xchacha20poly1305_ABYTES; // 17
static const byte TAG_INTERMEDIATE = crypto_secretstream_xchacha20poly1305_TAG_MESSAGE; // 0
__attribute_maybe_unused__ static const byte TAG_LAST = crypto_secretstream_xchacha20poly1305_TAG_FINAL; // 3

static const byte serverSignPublicKey[SERVER_SIGN_PUBLIC_KEY_SIZE] = {
    255, 23, 21, 243, 148, 177, 186, 0, 73, 34, 173, 130, 234, 251, 83, 130,
    138, 54, 215, 5, 170, 139, 175, 148, 71, 215, 74, 172, 27, 225, 26, 249
};

struct Crypto_t {
    byte serverPublicKey[CRYPTO_KEY_SIZE];
    byte clientPublicKey[CRYPTO_KEY_SIZE];
    byte clientSecretKey[CRYPTO_KEY_SIZE];
    byte clientKey[CRYPTO_KEY_SIZE];
    byte serverKey[CRYPTO_KEY_SIZE];
    crypto_secretstream_xchacha20poly1305_state clientDecryptionState;
    crypto_secretstream_xchacha20poly1305_state clientEncryptionState;
};

Crypto* cryptoInit(void) {
    if (sodium_init() < 0) return NULL;
    return (Crypto*) SDL_malloc(sizeof(Crypto));
}

bool cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey) {
    assert(crypto);
    int ckxk = crypto_kx_keypair(crypto->clientPublicKey, crypto->clientSecretKey);
    assert(ckxk);
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

unsigned cryptoEncryptedSize(unsigned unencryptedSize)
{ return unencryptedSize + ENCRYPTED_ADDITIONAL_BYTES_SIZE; }

const byte* cryptoClientPublicKey(const Crypto* crypto) {
    assert(crypto);
    return crypto->clientPublicKey;
}

byte* nullable cryptoEncrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize) {
    assert(crypto && bytes && bytesSize > 0);

    unsigned long long generatedEncryptedSize = 0;
    const typeof(generatedEncryptedSize) encryptedSize = cryptoEncryptedSize(bytesSize);
    byte* encrypted = SDL_malloc(encryptedSize);

    int result = crypto_secretstream_xchacha20poly1305_push(
        &(crypto->clientEncryptionState),
        encrypted,
        &generatedEncryptedSize,
        bytes,
        (typeof(generatedEncryptedSize)) bytesSize,
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

byte* nullable cryptoDecrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize) {
    assert(crypto && bytes && bytesSize > ENCRYPTED_ADDITIONAL_BYTES_SIZE);

    unsigned long long generatedDecryptedSize = 0;
    byte tag;
    const typeof(generatedDecryptedSize) decryptedSize = bytesSize - ENCRYPTED_ADDITIONAL_BYTES_SIZE;
    byte* decrypted = SDL_malloc(decryptedSize);

    int result = crypto_secretstream_xchacha20poly1305_pull(
        &(crypto->clientDecryptionState),
        decrypted,
        &generatedDecryptedSize,
        &tag,
        bytes,
        (typeof(generatedDecryptedSize)) bytesSize,
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
    assert(size > 0);
    randombytes_buf(filled, size);
}

void cryptoDestroy(Crypto* crypto) {
    assert(crypto);
    SDL_free(crypto);
}
