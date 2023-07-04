
#pragma once

#include <stdbool.h>
#include "defs.h"

extern const unsigned CRYPTO_KEY_SIZE;
extern const unsigned CRYPTO_SIGNATURE_SIZE;

struct Crypto_t; // implementation is hidden
typedef struct Crypto_t Crypto;

Crypto* nullable cryptoInit(void);
bool cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey); // returns true on success
const byte* cryptoEncryptionKey(const Crypto* crypto);
unsigned cryptoEncryptedSize(unsigned unencryptedSize);
const byte* cryptoClientPublicKey(const Crypto* crypto);
byte* nullable cryptoEncrypt(const byte* key, const byte* bytes, unsigned bytesSize); // returns mac (tag) + encrypted bytes + nonce
byte* nullable cryptoDecrypt(const byte* key, const byte* bytes, unsigned bytesSize); // consumes what is returned by encrypt
bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize);
void cryptoFillWithRandomBytes(byte* filled, unsigned size);
void cryptoDestroy(Crypto* crypto);
