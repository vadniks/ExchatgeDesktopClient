
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include "crypto.h"

staticAssert(crypto_kx_PUBLICKEYBYTES == crypto_box_PUBLICKEYBYTES);
staticAssert(crypto_kx_SECRETKEYBYTES == crypto_box_SECRETKEYBYTES);

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

byte* nullable crEncrypt(byte* bytes, unsigned size) {
//    byte nonce[crypto_box_NONCEBYTES];
//    randombytes_buf(nonce, sizeof nonce);
    byte* nonce = (byte*) "123456789012345678901234"; // TODO: randomize nonce for each session and add an exchange mechanism

    byte* encrypted = SDL_calloc(crypto_box_MACBYTES + size, sizeof(char));

    return crypto_box_easy(
        encrypted,
        bytes,
        size,
        nonce,
        this->serverPublicKey,
        this->clientSecretKey
    ) == 0 ? encrypted : NULL;
}

byte* crDecrypt(byte* bytes) {
    return NULL;
}

void crClean() {
    SDL_free(this);
}
