
#pragma once

#include <stdbool.h>
#include "defs.h"

bool netInit();
void netListen();
void netSend(byte* message, unsigned size);
void netClean();
