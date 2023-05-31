
#pragma once

#include <stdbool.h>
#include "defs.h"

byte* nullable crInit(byte* serverPublicKey);
unsigned crPublicKeySize();
byte* nullable crEncrypt(byte* bytes, unsigned size);
byte* crDecrypt(byte* bytes);
void crClean();
