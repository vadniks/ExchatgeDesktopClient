
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef struct {
    unsigned blockSize;
    unsigned unpaddedSize;
    unsigned paddedSize;
} crCryptDetails;

byte* nullable crInit(byte* serverPublicKey, crCryptDetails* cryptDetails);
unsigned crPublicKeySize();
byte* nullable crEncrypt(byte* bytes);
byte* nullable crDecrypt(byte* bytes);
void crClean();
