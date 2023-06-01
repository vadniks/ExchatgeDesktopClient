
#pragma once

#include <stdbool.h>
#include "defs.h"

byte* nullable crInit(byte* serverPublicKey);
void crSetNonce(byte* nonce);
unsigned crPublicKeySize();
unsigned crNonceSize();
byte* nullable crEncrypt(byte* bytes, unsigned size);
byte* nullable crDecrypt(byte* bytes, unsigned size);
void crClean();
