
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*MessageReceivedCallback)(const byte*); // don't deallocate parameter
typedef void (*NotifierCallback)(bool); // true on success
typedef void (*ServiceCallback)(int); // receives message's flag
typedef void (*Callback)(void); // just a callback

bool netInit(
    MessageReceivedCallback onMessageReceived,
    NotifierCallback onLogInResult,
    ServiceCallback onErrorReceived,
    Callback onDisconnected // cleanup is performed after this callback ends, so module needs to be reinitialized to keep working with it
); // returns true on success

void netLogIn(const char* username, const char* password);
unsigned netMessageBodySize(void);
void netListen(void);
void netSend(int flag, const byte* body, unsigned size, unsigned xTo);
void netClean(void);
