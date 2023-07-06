
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"
#include <sodium/sodium.h>
#include "defs.h"
#include <sdl/SDL_log.h>
int main(int argc, const char** argv) { // TODO: test only
    assert(sodium_init() >= 0); // TODO: ------------------------------- this shit is too complex to design & implement it improvising, so let's make a concept first ----------------------

    const unsigned keySize = crypto_secretstream_xchacha20poly1305_KEYBYTES;
    const unsigned headerSize = crypto_secretstream_xchacha20poly1305_HEADERBYTES;

    // client and server exchange their public keys to generate two shared keys
    byte serverKey[keySize];
    byte clientKey[keySize];
    randombytes_buf(serverKey, keySize);
    randombytes_buf(clientKey, keySize);

    // server (A) generates encoder stream (A doesn't need to decrypt what A encrypted)
    byte serverHeader[headerSize];
    crypto_secretstream_xchacha20poly1305_state serverEncryptionState;
    assert(!crypto_secretstream_xchacha20poly1305_init_push(&serverEncryptionState, serverHeader, (const byte*) serverKey));
    // and server sends his encoder stream header to client

    // client (B) generates decoder stream based on the server's header (B needs to decrypt what A encrypted)
    crypto_secretstream_xchacha20poly1305_state clientDecryptionState;
    assert(!crypto_secretstream_xchacha20poly1305_init_pull(&clientDecryptionState, (const byte*) serverHeader, (const byte*) serverKey));
    // then client generates his encoder stream (A needs to decrypt what B encrypted)
    byte clientHeader[headerSize];
    crypto_secretstream_xchacha20poly1305_state clientEncryptionState;
    assert(!crypto_secretstream_xchacha20poly1305_init_push(&clientEncryptionState, clientHeader, (const byte*) clientKey));
    // and client sends his encoder stream header to server

    // server generates decoder stream (A needs to decrypt what B encrypted)
    crypto_secretstream_xchacha20poly1305_state serverDecryptionState;
    assert(!crypto_secretstream_xchacha20poly1305_init_pull(&serverDecryptionState, (const byte*) clientHeader, (const byte*) clientKey));

    // at this point both client & server have their encoder/decoder streams to send/receive data
    const unsigned msgLen = 5, cipheredLen = msgLen + crypto_secretstream_xchacha20poly1305_ABYTES;
    byte ciphered[cipheredLen];
    byte msg[msgLen];
    byte xTag = 0;
    unsigned long long generatedLen;

#   define CHECK(x) for (unsigned i = 0; i < msgLen; msg[i++] == (byte) (x) ? (void) 0 : assert(0));

    // client sends smth
    SDL_memset(msg, '1', msgLen);
    assert(!crypto_secretstream_xchacha20poly1305_push(&clientEncryptionState, ciphered, &generatedLen, (const byte*) msg, (unsigned long long) msgLen, NULL, 0, xTag));
    SDL_Log("aa %u", generatedLen);
    assert((unsigned) generatedLen == cipheredLen);

    // server receives smth
    assert(!crypto_secretstream_xchacha20poly1305_pull(&serverDecryptionState, msg, &generatedLen, &xTag, (const byte*) ciphered, (unsigned long long) cipheredLen, NULL, 0));
    assert((unsigned) generatedLen == msgLen);
    assert(!xTag);
    SDL_Log("%.*s", msgLen, msg); // 11111
    CHECK('1')

    // server sends smth
    SDL_memset(msg, '2', msgLen);
    assert(!crypto_secretstream_xchacha20poly1305_push(&serverEncryptionState, ciphered, &generatedLen, (const byte*) msg, (unsigned long long) msgLen, NULL, 0, xTag));
    assert((unsigned) generatedLen == cipheredLen);

    // client receives smth
    assert(!crypto_secretstream_xchacha20poly1305_pull(&clientDecryptionState, msg, &generatedLen, &xTag, (const byte*) ciphered, (unsigned long long) cipheredLen, NULL, 0));
    assert((unsigned) generatedLen == msgLen);
    assert(!xTag);
    SDL_Log("%.*s", msgLen, msg); // 22222
    CHECK('2')

    // TODO: reached this point so this shit actually works! Now... how the f*** am I supposed to implement this in Go?

//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
//    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
