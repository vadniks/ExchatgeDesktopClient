
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
    crCryptDetails cryptDetails;
)

static byte* nullable encrypt(byte* bytes, unsigned bytesSize); // TODO: test only
static byte* nullable decrypt(byte* bytes, unsigned bytesSize);

byte* nullable crInit(byte* serverPublicKey, crCryptDetails* cryptDetails) {
    if (sodium_init() < 0) {
//        SDL_free(serverPublicKey);
        SDL_free(cryptDetails);
        return NULL;
    }

    serverPublicKey = SDL_malloc(crypto_kx_PUBLICKEYBYTES); // TODO: test only
    byte* serverSecretKey = SDL_malloc(crypto_kx_SECRETKEYBYTES);
    crypto_kx_keypair(serverPublicKey, serverSecretKey);
    SDL_free(serverSecretKey);

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

    this->cryptDetails.blockSize = cryptDetails->blockSize;
    this->cryptDetails.unpaddedSize = cryptDetails->unpaddedSize;
    this->cryptDetails.paddedSize = cryptDetails->paddedSize;
    SDL_free(cryptDetails);

    const int msgSize = 1048; // TODO: test only
    const int encryptedMsgSize = msgSize + 16 + 24; // mac + nonce, without padding for now

    const char* test = "test";
    byte* msg = SDL_calloc(msgSize, sizeof(char));
    SDL_memcpy(msg, test, 4);

    printf("aa\n");
    for (int i = 0; i < msgSize; printf("%u ", msg[i++]));
    printf("\n");

    byte* encrypted = encrypt(msg, msgSize);
    if (!encrypted) return false;

    printf("a\n");
    for (int i = 0; i < encryptedMsgSize; printf("%u ", encrypted[i++]));
    printf("\nb\n");

    byte* decrypted = decrypt(encrypted, encryptedMsgSize);
    if (!decrypted) return false;

    printf("%s\n", decrypted); // TODO: it finally works
    for (int i = 0; i < msgSize; printf("%u ", decrypted[i++]));
    SDL_free(decrypted);
    printf("\n");

    return this->clientPublicKey;
}

unsigned crPublicKeySize() { return crypto_kx_PUBLICKEYBYTES; }

static byte* nullable addPadding(byte* bytes) {
    byte* padded = SDL_malloc(this->cryptDetails.paddedSize);
    SDL_memcpy(padded, bytes, this->cryptDetails.unpaddedSize);
    SDL_free(bytes);

    unsigned long generatedPaddedSize = 0;
    sodium_pad(
        &generatedPaddedSize,
        padded,
        this->cryptDetails.unpaddedSize,
        this->cryptDetails.blockSize,
        this->cryptDetails.paddedSize
    );

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

byte* nullable crEncrypt(byte* bytes) {
    byte* padded = addPadding(bytes);
    if (!padded) return NULL;
    return encrypt(bytes, this->cryptDetails.paddedSize);
}

static byte* nullable decrypt(byte* bytes, unsigned bytesSize) {
    const unsigned decryptedSize = bytesSize - crypto_secretbox_MACBYTES - crypto_secretbox_NONCEBYTES;
    byte* decrypted = SDL_calloc(decryptedSize, sizeof(char));
    const unsigned encryptedAndTagSize = bytesSize - crypto_secretbox_NONCEBYTES;

    byte* result = crypto_secretbox_open_easy(
        decrypted,
        bytes,
        encryptedAndTagSize,
        bytes + encryptedAndTagSize,
        this->clientSendKey // TODO: receive key
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
    sodium_unpad(
        &generatedUnpaddedSize,
        padded,
        this->cryptDetails.paddedSize,
        this->cryptDetails.blockSize
    );

    if (generatedUnpaddedSize != this->cryptDetails.unpaddedSize) {
        SDL_free(padded);
        return NULL;
    }

    byte* unpadded = SDL_malloc(this->cryptDetails.unpaddedSize);
    SDL_memcpy(unpadded, padded, this->cryptDetails.unpaddedSize);
    SDL_free(padded);

    return unpadded;
}

byte* nullable crDecrypt(byte* bytes) {
    byte* decrypted = decrypt(bytes, NET_RECEIVE_BUFFER_SIZE);
    if (!decrypted) return NULL;
    return removePadding(decrypted);
}

void crClean() {
    SDL_free(this);
}
