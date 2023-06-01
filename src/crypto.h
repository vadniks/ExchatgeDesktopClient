
#pragma once

#include <stdbool.h>
#include "defs.h"

byte* nullable crInit(byte* serverPublicKey, unsigned maxEncryptedSize);
unsigned crPublicKeySize();
byte* nullable crEncrypt(byte* bytes, unsigned size);
byte* nullable crDecrypt(byte* bytes, unsigned size);
void crClean();
