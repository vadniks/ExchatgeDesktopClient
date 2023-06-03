
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include <assert.h>
#include "crypto.h"

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_secretbox_KEYBYTES);

typedef struct {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"
    CryptoCryptDetails;
#pragma clang diagnostic pop

    unsigned paddedSize;
} CryptoCryptDetailsInternal;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    byte serverPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientSecretKey[crypto_kx_SECRETKEYBYTES];
    byte clientReceiveKey[crypto_kx_SESSIONKEYBYTES];
    byte clientSendKey[crypto_kx_SESSIONKEYBYTES];
    CryptoCryptDetailsInternal cryptDetails;
)
#pragma clang diagnostic pop

byte* nullable cryptoInit(byte* serverPublicKey, CryptoCryptDetails* cryptDetails) {
    assert(cryptDetails->blockSize > 0 && cryptDetails->unpaddedSize > 0);
    assert(sodium_init() >= 0);

    this = SDL_malloc(sizeof *this);
    SDL_memcpy(this->serverPublicKey, serverPublicKey, cryptoPublicKeySize());
    SDL_free(serverPublicKey);

    this->cryptDetails.blockSize = cryptDetails->blockSize;
    this->cryptDetails.unpaddedSize = cryptDetails->unpaddedSize;
    this->cryptDetails.paddedSize = cryptoPaddedSize();
    SDL_free(cryptDetails);

    crypto_kx_keypair(this->clientPublicKey, this->clientSecretKey);

    if (crypto_kx_client_session_keys(
        this->clientReceiveKey,
        this->clientSendKey,
        this->clientPublicKey,
        this->clientSecretKey,
        this->serverPublicKey
    ) != 0) {
        cryptoClean();
        return NULL;
    }

    printf("rx: "); // TODO: test only
    for (unsigned i = 0; i < cryptoPublicKeySize(); i++) printf("%d ", this->clientReceiveKey[i]);
    printf("tx: ");
    for (unsigned i = 0; i < cryptoPublicKeySize(); i++) printf("%d ", this->clientSendKey[i]);
    printf("\n");

    return this->clientPublicKey;
}

unsigned cryptoPublicKeySize() { return crypto_kx_PUBLICKEYBYTES; }

unsigned cryptoEncryptedSize() {
    return this->cryptDetails.paddedSize
        + crypto_secretbox_MACBYTES
        + crypto_secretbox_NONCEBYTES;
}

unsigned cryptoPaddedSize() { // 1056
    const int dividend = (int) this->cryptDetails.unpaddedSize,
        divider = (int) this->cryptDetails.blockSize;

    const div_t quotient = div(!dividend ? 0 : dividend + 1, divider);
    return divider * (quotient.quot + (quotient.rem > 0 ? 1 : 0));
}

static byte* nullable addPadding(byte* bytes) {
    byte* padded = SDL_malloc(this->cryptDetails.paddedSize);
    SDL_memcpy(padded, bytes, this->cryptDetails.unpaddedSize);
    SDL_free(bytes);

    unsigned long generatedPaddedSize = 0;
    if (sodium_pad(
        &generatedPaddedSize,
        padded,
        this->cryptDetails.unpaddedSize,
        this->cryptDetails.blockSize,
        this->cryptDetails.paddedSize
    ) != 0) {
        SDL_free(padded);
        return NULL;
    }

    if (generatedPaddedSize != (unsigned long) this->cryptDetails.paddedSize) {
        SDL_free(padded);
        return NULL;
    }
    return padded;
}

static byte* nullable encrypt(byte* bytes, unsigned bytesSize) {
    const unsigned encryptedSize = bytesSize + crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
    byte* encrypted = SDL_calloc(encryptedSize, sizeof(char));

    byte* nonceStart = encrypted + encryptedSize - crypto_secretbox_NONCEBYTES;
    randombytes_buf(nonceStart, crypto_secretbox_NONCEBYTES);

    byte* result = crypto_secretbox_easy(
        encrypted,
        bytes,
        bytesSize,
        nonceStart,
        this->clientSendKey
    ) == 0 ? encrypted : NULL;

    if (!result) SDL_free(encrypted);
    SDL_free(bytes);
    return result;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult" // SAT thinks this Function always return null, but it really doesn't

byte* nullable cryptoEncrypt(byte* bytes) {
    byte* padded = addPadding(bytes);
    if (!padded) return NULL;
    return encrypt(padded, this->cryptDetails.paddedSize);
}

#pragma clang diagnostic pop

static byte* nullable decrypt(byte* bytes, unsigned bytesSize) {
    const unsigned decryptedSize = bytesSize - crypto_secretbox_MACBYTES - crypto_secretbox_NONCEBYTES;
    byte* decrypted = SDL_calloc(decryptedSize, sizeof(char));
    const unsigned encryptedAndTagSize = bytesSize - crypto_secretbox_NONCEBYTES;

    byte* result = crypto_secretbox_open_easy(
        decrypted,
        bytes,
        encryptedAndTagSize,
        bytes + encryptedAndTagSize,
        this->clientReceiveKey
    ) == 0 ? decrypted : NULL;

    if (!result) SDL_free(decrypted);
    SDL_free(bytes);
    return result;
}

static byte* nullable removePadding(byte* bytes) {
    byte* padded = SDL_malloc(this->cryptDetails.paddedSize);
    SDL_memcpy(padded, bytes, this->cryptDetails.paddedSize);
    SDL_free(bytes);

    unsigned long generatedUnpaddedSize = 0;
    if (sodium_unpad(
        &generatedUnpaddedSize,
        padded,
        this->cryptDetails.paddedSize,
        this->cryptDetails.blockSize
    ) != 0) {
        SDL_free(padded);
        return NULL;
    }

    if (generatedUnpaddedSize != this->cryptDetails.unpaddedSize) {
        SDL_free(padded);
        return NULL;
    }

    byte* unpadded = SDL_malloc(this->cryptDetails.unpaddedSize);
    SDL_memcpy(unpadded, padded, this->cryptDetails.unpaddedSize);
    SDL_free(padded);

    return unpadded;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult" // SAT thinks this Function always return null, but it really doesn't

byte* nullable cryptoDecrypt(byte* bytes) {
    byte* decrypted = decrypt(bytes,
        this->cryptDetails.paddedSize + (int) crypto_secretbox_MACBYTES + (int) crypto_secretbox_NONCEBYTES);
    if (!decrypted) return NULL;
    return removePadding(decrypted);
}

#pragma clang diagnostic pop

void cryptoClean() { SDL_free(this); }
