
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef struct {
    unsigned blockSize;
    unsigned unpaddedSize;
} CryptoCryptDetails;

byte* nullable cryptoInit(byte* serverPublicKey, CryptoCryptDetails* cryptDetails);
unsigned cryptoPublicKeySize();
unsigned cryptoEncryptedSize();
unsigned cryptoPaddedSize();
byte* nullable cryptoEncrypt(byte* bytes); // returns mac (tag) + encrypted bytes + nonce
byte* nullable cryptoDecrypt(byte* bytes); // consumes what is returned by encrypt
void cryptoClean();
