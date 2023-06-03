
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef struct {
    unsigned blockSize;
    unsigned unpaddedSize;
} CryptoCryptDetails;

byte* nullable cryptoInit(byte* serverPublicKey, CryptoCryptDetails* cryptDetails); // returns client public key
unsigned cryptoPublicKeySize();
unsigned cryptoEncryptedSize(); // returns size of mac (16) + encrypted bytes (cryptoDetails.unpaddedSize) + nonce (24)
unsigned cryptoPaddedSize(); // returns size of unencrypted padded size
byte* nullable cryptoEncrypt(byte* bytes); // returns mac (tag) + encrypted bytes + nonce
byte* nullable cryptoDecrypt(byte* bytes); // consumes what is returned by encrypt
void cryptoClean();
