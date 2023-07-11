
#pragma once

#include <stdbool.h>
#include "defs.h"

extern const unsigned CRYPTO_KEY_SIZE;
extern const unsigned CRYPTO_HEADER_SIZE;
extern const unsigned CRYPTO_SIGNATURE_SIZE;

struct Crypto_t;
typedef struct Crypto_t Crypto;

Crypto* nullable cryptoInit(void);
bool cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey); // returns true on success
byte* nullable cryptoInitializeCoderStreams(Crypto* crypto, const byte* serverStreamHeader); // expects a HEADER-SIZE-sized server header's bytes, returns a deallocation-required HEADER-SIZE-sized client header's bytes on success and null otherwise
unsigned cryptoEncryptedSize(unsigned unencryptedSize);
const byte* cryptoClientPublicKey(const Crypto* crypto);
byte* nullable cryptoEncrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize); // returns encryptedSize()-sized encrypted bytes
byte* nullable cryptoDecrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize); // consumes what is returned by encrypt
bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize);
void cryptoFillWithRandomBytes(byte* filled, unsigned size);
void cryptoDestroy(Crypto* crypto);
