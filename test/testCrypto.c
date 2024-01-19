
#include <assert.h>
#include <SDL.h>
#include "../src/crypto.h"
#include "testCrypto.h"

void testCrypto_start(void) { cryptoInit(); }
void testCrypto_stop(void) { cryptoClean(); }

void testCrypto_keyExchange(void) {
    const int allocations = SDL_GetNumAllocations();

    CryptoKeys* serverKeys = cryptoKeysInit();
    const byte* serverPublicKey = cryptoGenerateKeyPairAsServer(serverKeys);

    CryptoKeys* clientKeys = cryptoKeysInit();
    assert(cryptoExchangeKeys(clientKeys, serverPublicKey));

    assert(cryptoExchangeKeysAsServer(serverKeys, cryptoClientPublicKey(clientKeys)));

    const void* sharedServerKeys = (void*) serverKeys + CRYPTO_KEY_SIZE * 3;
    const void* sharedClientKeys = (void*) clientKeys + CRYPTO_KEY_SIZE * 3;
    const unsigned sharedKeysSize = CRYPTO_KEY_SIZE * 2;

    assert(!SDL_memcmp(sharedServerKeys, sharedClientKeys, sharedKeysSize));
    assert(!SDL_memcmp(exposedTestCrypto_sharedDecryptionKey(clientKeys), exposedTestCrypto_sharedDecryptionKey(serverKeys), CRYPTO_KEY_SIZE));
    assert(!SDL_memcmp(exposedTestCrypto_sharedEncryptionKey(clientKeys), exposedTestCrypto_sharedEncryptionKey(serverKeys), CRYPTO_KEY_SIZE));
    assert(!SDL_memcmp(sharedServerKeys, exposedTestCrypto_sharedEncryptionKey(clientKeys), CRYPTO_KEY_SIZE));
    assert(!SDL_memcmp(sharedClientKeys + CRYPTO_KEY_SIZE, exposedTestCrypto_sharedDecryptionKey(serverKeys), CRYPTO_KEY_SIZE));

    cryptoKeysDestroy(serverKeys);
    cryptoKeysDestroy(clientKeys);

    assert(allocations == SDL_GetNumAllocations());
}
