
#pragma once

#include <stdbool.h>
#include "defs.h"

extern const unsigned CRYPTO_KEY_SIZE;

struct Crypto_t; // implementation is hidden
typedef struct Crypto_t Crypto;

Crypto* nullable cryptoInit(void);
bool cryptoExchangeKeys(Crypto* crypto); // returns true on success
void cryptoSetServerPublicKey(Crypto* crypto, const byte* key);
void cryptoSetEncryptionKey(Crypto* crypto, const byte* key); // sets permanent key that was generated & exchanged before
unsigned cryptoEncryptedSize(unsigned unencryptedSize);
byte* cryptoClientPublicKey(Crypto* crypto); // don't deallocate result
byte* nullable cryptoEncrypt(const Crypto* crypto, const byte* bytes, unsigned bytesSize); // returns mac (tag) + encrypted bytes + nonce
byte* nullable cryptoDecrypt(const Crypto* crypto, const byte* bytes, unsigned bytesSize); // consumes what is returned by encrypt
void cryptoDestroy(Crypto* crypto);
