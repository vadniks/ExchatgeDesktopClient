
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*MessageReceivedCallback)(const byte*); // don't deallocate parameter
typedef void (*NotifierCallback)(bool); // true on success

bool netInit(
    MessageReceivedCallback onMessageReceived,
    NotifierCallback onLogInResult
); // returns true on success

void netLogIn(void);
unsigned netMessageSize(void);
void netListen(void);
void netSend(int flag, const byte* body, unsigned size, unsigned xTo);
void netClean(void);
