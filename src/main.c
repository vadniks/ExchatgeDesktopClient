
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
    const unsigned msgLen = 5;
    byte ciphered[msgLen];
    byte msg[msgLen];
    byte xTag = 0;

#   define CHECK(x) for (unsigned i = 0; i < msgLen; msg[i++] == (byte) (x) ? (void) 0 : assert(0));

    // client sends smth
    SDL_memset(msg, '1', msgLen);
    assert(!crypto_secretstream_xchacha20poly1305_push(&clientEncryptionState, ciphered, NULL, (const byte*) msg, msgLen, NULL, 0, xTag));

    // server receives smth
    assert(!crypto_secretstream_xchacha20poly1305_pull(&serverDecryptionState, msg, NULL, &xTag, (const byte*) ciphered, msgLen, NULL, 0)); // TODO: not working
    assert(!xTag);
    SDL_Log("%.*s", msgLen, msg); // 11111
    CHECK('1')

    // server sends smth
    SDL_memset(msg, '2', msgLen);
    assert(!crypto_secretstream_xchacha20poly1305_push(&serverEncryptionState, ciphered, NULL, (const byte*) msg, msgLen, NULL, 0, xTag));

    // client receives smth
    assert(!crypto_secretstream_xchacha20poly1305_pull(&clientDecryptionState, msg, NULL, &xTag, (const byte*) ciphered, msgLen, NULL, 0));
    assert(!xTag);
    SDL_Log("%.*s", msgLen, msg); // 22222
    CHECK('2')

    // ...

    ////////////////////////////////////////////////////////////////////////////////////////////

    unsigned len = 10;
    const char* text1 = "aaaaaaaaaa";
    const char* text2 = "bbbbbbbbbb";
    const char* text3 = "cccccccccc";

    byte key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    randombytes_buf(key, sizeof key);

    byte header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state encryptState;
    assert(!crypto_secretstream_xchacha20poly1305_init_push(&encryptState, header, key));

    unsigned encryptedLen = len + crypto_secretstream_xchacha20poly1305_ABYTES;
    unsigned long long generatedEncryptedLen;
    unsigned long long generatedDecryptedLen;
    byte tag = 0;
    byte encrypted1[encryptedLen];
    byte encrypted2[encryptedLen];
    byte encrypted3[encryptedLen];

    assert(!crypto_secretstream_xchacha20poly1305_push(&encryptState, encrypted1, &generatedEncryptedLen, (const byte*) text1, len, NULL, 0, tag));
    assert(generatedEncryptedLen == encryptedLen);

    crypto_secretstream_xchacha20poly1305_state decryptState;
    assert(!crypto_secretstream_xchacha20poly1305_init_pull(&decryptState, header, key));

    char decrypted1[len];
    char decrypted2[len];
    char decrypted3[len];

    assert(!crypto_secretstream_xchacha20poly1305_pull(&decryptState, (byte*) decrypted1, &generatedDecryptedLen, &tag, encrypted1, encryptedLen, NULL, 0));
    assert(generatedDecryptedLen == len);
    assert(!tag);

    SDL_Log("%.*s", len, decrypted1);

    assert(!crypto_secretstream_xchacha20poly1305_push(&decryptState, encrypted2, &generatedEncryptedLen, (const byte*) text2, len, NULL, 0, tag));
    assert(generatedEncryptedLen == encryptedLen);

    assert(!crypto_secretstream_xchacha20poly1305_pull(&encryptState, (byte*) decrypted2, &generatedDecryptedLen, &tag, encrypted2, encryptedLen, NULL, 0));
    assert(generatedDecryptedLen == len);
    assert(!tag);

    SDL_Log("%.*s", len, decrypted2);

    assert(!crypto_secretstream_xchacha20poly1305_push(&decryptState, encrypted3, &generatedEncryptedLen, (const byte*) text3, len, NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL));
    assert(generatedEncryptedLen == encryptedLen);

    assert(!crypto_secretstream_xchacha20poly1305_pull(&encryptState, (byte*) decrypted3, &generatedDecryptedLen, &tag, encrypted3, encryptedLen, NULL, 0));
    assert(generatedDecryptedLen == len);
    assert(tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    // crypto_secretstream_xchacha20poly1305_TAG_FINAL

    SDL_Log("%.*s", len, decrypted3);

//    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
//    lifecycleLoop();
//    lifecycleClean();
//    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
