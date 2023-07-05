
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"
#include <sodium/sodium.h>
#include "defs.h"
#include <sdl/SDL_log.h>
int main(int argc, const char** argv) {
    unsigned len = 10;
    const char* text1 = "aaaaaaaaaa";
    const char* text2 = "bbbbbbbbbb";

    assert(sodium_init() >= 0);

    byte key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    randombytes_buf(key, sizeof key);

    byte header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state encryptState;
    crypto_secretstream_xchacha20poly1305_init_push(&encryptState, header, key);

    unsigned encryptedLen = len + crypto_secretstream_xchacha20poly1305_ABYTES;
    byte tag = 0;
    byte encrypted1[encryptedLen];
    byte encrypted2[encryptedLen];

    crypto_secretstream_xchacha20poly1305_push(&encryptState, encrypted1, NULL, (const byte*) text1, len, NULL, 0, tag);

    crypto_secretstream_xchacha20poly1305_state decryptState;
    assert(!crypto_secretstream_xchacha20poly1305_init_pull(&decryptState, header, key));

    char decrypted1[len];
    char decrypted2[len];

    assert(!crypto_secretstream_xchacha20poly1305_pull(&decryptState, (byte*) decrypted1, NULL, &tag, encrypted1, encryptedLen, NULL, 0));
    assert(!tag);

    SDL_Log("%.*s", len, decrypted1);

    crypto_secretstream_xchacha20poly1305_push(&encryptState, encrypted2, NULL, (const byte*) text2, len, NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    assert(!crypto_secretstream_xchacha20poly1305_pull(&decryptState, (byte*) decrypted2, NULL, &tag, encrypted2, encryptedLen, NULL, 0));
    assert(tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    SDL_Log("%.*s", len, decrypted2);



//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
//    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
