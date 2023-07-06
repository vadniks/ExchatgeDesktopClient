
#pragma once

#include <stdbool.h>
#include "defs.h"

extern const unsigned CRYPTO_KEY_SIZE;
extern const unsigned CRYPTO_HEADER_SIZE;
extern const unsigned CRYPTO_SIGNATURE_SIZE;

struct Crypto_t;
typedef struct Crypto_t Crypto;

Crypto* nullable cryptoInit(void);
byte* nullable cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey); // returns headers bytes of size 2 * HEADER_SIZE for encryption and decryption on success
unsigned cryptoEncryptedSize(unsigned unencryptedSize);
const byte* cryptoClientPublicKey(const Crypto* crypto);
byte* nullable cryptoEncrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize);
byte* nullable cryptoDecrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize);
bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize);
void cryptoFillWithRandomBytes(byte* filled, unsigned size);
void cryptoDestroy(Crypto* crypto);
