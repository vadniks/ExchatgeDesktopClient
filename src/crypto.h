
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef struct {
    unsigned blockSize;
    unsigned unpaddedSize;
} CrCryptDetails;

byte* nullable crInit(byte* serverPublicKey, CrCryptDetails* cryptDetails);
unsigned crPublicKeySize();
unsigned crEncryptedSize();
unsigned crPaddedSize();
byte* nullable crEncrypt(byte* bytes); // returns mac (tag) + encrypted bytes + nonce
byte* nullable crDecrypt(byte* bytes); // consumes what is returned by encrypt
void crClean();
