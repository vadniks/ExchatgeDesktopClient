
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include "crypto.h"

THIS(
    byte serverPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientPublicKey[crypto_kx_PUBLICKEYBYTES];
    byte clientSecretKey[crypto_kx_SECRETKEYBYTES];
    byte clientReceiveKey[crypto_kx_SESSIONKEYBYTES];
    byte clientSendKey[crypto_kx_SESSIONKEYBYTES];
)

byte* nullable crInit(byte* serverPublicKey) {
    if (sodium_init() < 0) return NULL;

    this = SDL_malloc(sizeof *this);
    SDL_memcpy(this->serverPublicKey, serverPublicKey, crypto_kx_PUBLICKEYBYTES);
    SDL_free(serverPublicKey);

    crypto_kx_keypair(this->clientPublicKey, this->clientSecretKey);

    if (crypto_kx_client_session_keys(
        this->clientReceiveKey,
        this->clientSendKey,
        this->clientPublicKey,
        this->clientSecretKey,
        this->serverPublicKey
    ) != 0)
        return false;

    return this->clientPublicKey;
}

unsigned crPublicKeySize() { return crypto_kx_PUBLICKEYBYTES; }

byte* crEncrypt(byte* bytes) {
    return NULL;
}

byte* crDecrypt(byte* bytes) {
    return NULL;
}

void crClean() {
    SDL_free(this);
}
