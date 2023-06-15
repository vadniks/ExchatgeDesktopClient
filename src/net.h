
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*MessageReceivedCallback)(const byte*); // don't deallocate parameter

bool netInit(MessageReceivedCallback onMessageReceived); // returns true on success
unsigned netMessageSize(void);
void netListen(void);
void netSend(const byte* bytes, unsigned size);
void netClean(void);
