
#pragma once

#include <stdbool.h>
#include "defs.h"

byte* nullable crInit(byte* serverPublicKey);
unsigned crPublicKeySize();
byte* crEncrypt(byte* bytes);
byte* crDecrypt(byte* bytes);
void crClean();
