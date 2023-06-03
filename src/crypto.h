
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
byte* nullable crEncrypt(byte* bytes);
byte* nullable crDecrypt(byte* bytes);
void crClean();
