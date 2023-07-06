



// ---------------------------------------------- TODO -----------------------------------------------------------------





#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"
#include <sodium/sodium.h>
#include "defs.h"
#include <sdl/SDL_log.h>
#include <stdio.h>
int main(int argc, const char** argv) { // TODO: test only
    assert(sodium_init() >= 0); // TODO: ------------------------------- this shit is too complex to design & implement it improvising, so let's make a concept first ----------------------

    const unsigned keySize = crypto_secretstream_xchacha20poly1305_KEYBYTES;
    const unsigned headerSize = crypto_secretstream_xchacha20poly1305_HEADERBYTES;

    // client and server create buffers for encryption/decryption shared keys via key exchanging
    byte clientKey[keySize];
    byte serverKey[keySize];

    // server generates his key pair
    byte serverPublicKey[keySize], serverSecretKey[keySize];
    crypto_kx_keypair(serverPublicKey, serverSecretKey);
    // then server sends his public key to client

#   define PRINT(x) printf(#x ": "); for (unsigned i = 0; i < keySize; printf("%u ", ((const byte*) (x))[i++])); printf("\n");

    // client generates his key pair, receives server's public key & generates two shared keys based on server's public key
    byte clientPublicKey[keySize], clientSecretKey[keySize];
    crypto_kx_keypair(clientPublicKey, clientSecretKey);
    assert(!crypto_kx_client_session_keys(clientKey, serverKey, clientPublicKey, clientSecretKey, serverPublicKey));
    PRINT(clientKey) PRINT(serverKey)
    // then client sends his public key to server

    // server receives client's public key & generates two shared keys based on client's public key
    assert(!crypto_kx_server_session_keys(serverKey, clientKey, serverPublicKey, serverSecretKey, clientPublicKey));
    PRINT(clientKey) PRINT(serverKey)

    // TODO: reached this point - key exchange successful, we can proceed to the next part

    // now client & server create encoder/decoder streams

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
