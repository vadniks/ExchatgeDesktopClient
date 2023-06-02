
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef struct {
    unsigned blockSize;
    unsigned unpaddedSize; // decrypted & unpadded
    unsigned paddedSize; // decrypted & padded
} crCryptDetails;

byte* nullable crInit(byte* serverPublicKey, crCryptDetails* cryptDetails);
unsigned crPublicKeySize();
unsigned crServiceSectionSize();
byte* nullable crEncrypt(byte* bytes);
byte* nullable crDecrypt(byte* bytes);
void crClean();
