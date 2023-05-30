
#pragma once

#include <stdbool.h>
#include "defs.h"

bool crInit(byte* serverPublicKey);
__attribute_pure__ unsigned crPublicKeySize();
byte* crEncrypt(byte* bytes);
byte* crDecrypt(byte* bytes);
void crClean();
