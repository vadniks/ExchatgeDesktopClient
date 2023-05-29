
#pragma once

#include <stdbool.h>
#include "defs.h"

__attribute_deprecated__ char* net();
bool ntInit();
void ntSend(byte* message);
byte* ntReceive();
void ntClean();
