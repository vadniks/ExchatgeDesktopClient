
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include "crypto.h"
#include <stdio.h> // TODO: test only

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_box_PUBLICKEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_box_SECRETKEYBYTES);

THIS(
    byte serverPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientSecretKey[crypto_kx_SECRETKEYBYTES];
    byte clientReceiveKey[crypto_kx_SESSIONKEYBYTES];
    byte clientSendKey[crypto_kx_SESSIONKEYBYTES];
    byte nonce[crypto_secretbox_NONCEBYTES];
)

byte* nullable crInit(byte* serverPublicKey) {
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

    return this->clientPublicKey;
}

void crSetNonce(byte* nonce) {
    SDL_memcpy(this->nonce, nonce, crNonceSize());
    SDL_free(nonce);
}

unsigned crPublicKeySize() { return crypto_kx_PUBLICKEYBYTES; }
unsigned crNonceSize() { return crypto_secretbox_NONCEBYTES; }

byte* nullable crEncrypt(byte* bytes, unsigned size) {
    byte* encrypted = SDL_calloc(crypto_box_MACBYTES + size, sizeof(char));

    byte* result = crypto_secretbox_easy(
        encrypted,
        bytes,
        size,
        this->nonce,
        this->clientSendKey
    ) == 0 ? encrypted : NULL;

    if (!result) SDL_free(encrypted);
    SDL_free(bytes);
    return result;
}

byte* crDecrypt(byte* bytes) {
    return NULL;
}

void crClean() {
    SDL_free(this);
}
