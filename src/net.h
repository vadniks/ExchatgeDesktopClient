
#pragma once

#include <stdbool.h>
#include "defs.h"

bool ntInit();
void ntListen();
void ntSend(byte* message, unsigned size);
void ntClean();
