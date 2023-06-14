
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*MessageReceivedCallback)(const byte*);

bool netInit(MessageReceivedCallback onMessageReceived); // returns true on success
unsigned netMessageSize();
void netListen();
void netSend(const byte* bytes, unsigned size);
void netClean();
