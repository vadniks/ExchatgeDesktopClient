
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*MessageReceivedCallback)(const byte*);
typedef void (*NotifierCallback)(bool); // true on success
typedef void (*ServiceCallback)(int); // receives message's flag
typedef void (*Callback)(void); // just a callback

bool netInit(
    MessageReceivedCallback onMessageReceived,
    NotifierCallback onLogInResult,
    ServiceCallback onErrorReceived,
    NotifierCallback onRegisterResult,
    Callback onDisconnected // cleanup is performed after this callback ends, so module needs to be reinitialized to continue working with it
); // returns true on success

void netLogIn(const char* username, const char* password); // in case of failure the server disconnects client
void netRegister(const char* username, const char* password); // the server disconnects client regardless of the result, but it sends messages with the result
unsigned netMessageBodySize(void);
void netListen(void);
void netSend(int flag, const byte* body, unsigned size, unsigned xTo);
void netClean(void);
