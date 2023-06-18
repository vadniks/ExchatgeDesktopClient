
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*MessageReceivedCallback)(const byte*); // don't deallocate parameter
typedef void (*NotifierCallback)(void);

bool netInit(
    MessageReceivedCallback onMessageReceived,
    NotifierCallback onLogInFailed
); // returns true on success

unsigned netMessageSize(void);
void netListen(void);
void netSend(int flag, const byte* body, unsigned size, unsigned xTo);
void netClean(void);
