
#pragma once

#include <stdbool.h>
#include "defs.h"

bool crInit(byte* publicKey);
void crGenerateKeypair();
byte* crEncrypt(byte* bytes);
byte* crDecrypt(byte* bytes);
void crClean();
