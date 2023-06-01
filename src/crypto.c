
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include "crypto.h"
#include <stdio.h> // TODO: test only

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_secretbox_KEYBYTES);

THIS(
    byte serverPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientSecretKey[crypto_kx_SECRETKEYBYTES];
    byte clientReceiveKey[crypto_kx_SESSIONKEYBYTES];
    byte clientSendKey[crypto_kx_SESSIONKEYBYTES];
    unsigned maxEncryptedSize;
)

byte* nullable crInit(byte* serverPublicKey, unsigned maxEncryptedSize) {
    if (sodium_init() < 0) return NULL;

    this = SDL_malloc(sizeof *this);
    SDL_memcpy(this->serverPublicKey, serverPublicKey, crPublicKeySize());
    SDL_free(serverPublicKey);

    crypto_kx_keypair(this->clientPublicKey, this->clientSecretKey);

    if (crypto_kx_client_session_keys(
        this->clientReceiveKey,
        this->clientSendKey,
        this->clientPublicKey,
        this->clientSecretKey,
        this->serverPublicKey
    ) != 0)
        return NULL;

    printf("rx: "); // TODO: test only
    for (unsigned i = 0; i < crPublicKeySize(); i++) printf("%d ", this->clientReceiveKey[i]);
    printf("tx: ");
    for (unsigned i = 0; i < crPublicKeySize(); i++) printf("%d ", this->clientSendKey[i]);
    printf("\n");

    this->maxEncryptedSize = maxEncryptedSize;
    return this->clientPublicKey;
}

unsigned crPublicKeySize() { return crypto_kx_PUBLICKEYBYTES; }

byte* nullable crEncrypt(byte* bytes, unsigned size) { // TODO: add padding to hide original message's length
    const unsigned encryptedSize = crypto_secretbox_MACBYTES + size + crypto_secretbox_NONCEBYTES;
    if (encryptedSize > this->maxEncryptedSize) { // TODO: implement this properly on the server side
        SDL_free(bytes);
        return NULL;
    }

    byte* encrypted = SDL_calloc(encryptedSize, sizeof(char));
    byte* nonceStart = encrypted + crypto_secretbox_MACBYTES + size;
    randombytes_buf(nonceStart, crypto_secretbox_NONCEBYTES);

    byte* result = crypto_secretbox_easy(
        encrypted,
        bytes,
        size,
        nonceStart,
        this->clientSendKey
    ) == 0 ? encrypted : NULL;

    if (!result) SDL_free(encrypted);
    SDL_free(bytes);
    return result;
}

byte* nullable crDecrypt(byte* bytes, unsigned size) {
    const unsigned decryptedSize = size - crypto_secretbox_MACBYTES - crypto_secretbox_NONCEBYTES;
    byte* decrypted = SDL_calloc(decryptedSize, sizeof(char));

    byte* result = crypto_secretbox_open_easy(
        decrypted,
        bytes,
        size,
        bytes + crypto_secretbox_MACBYTES + decryptedSize,
        this->clientReceiveKey
    ) == 0 ? decrypted : NULL;

    if (!result) SDL_free(decrypted);
    SDL_free(bytes);
    return NULL;
}

void crClean() {
    SDL_free(this);
}
