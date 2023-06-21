
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*NetMessageReceivedCallback)(const byte*);
typedef void (*NetNotifierCallback)(bool); // true on success
typedef void (*NetServiceCallback)(int); // receives message's flag
typedef void (*NetCallback)(void);

extern const unsigned NET_USERNAME_SIZE;
extern const unsigned NET_UNHASHED_PASSWORD_SIZE;

bool netInit(
    NetMessageReceivedCallback onMessageReceived,
    NetNotifierCallback onLogInResult,
    NetServiceCallback onErrorReceived, // not called on login error & register error as there are separated callback for them
    NetNotifierCallback onRegisterResult,
    NetCallback onDisconnected // cleanup is performed before this callback starts, so module needs to be reinitialized after this callback ends to continue working with this module
); // returns true on success

void netLogIn(const char* username, const char* password); // in case of failure the server disconnects client
void netRegister(const char* username, const char* password); // the server disconnects client regardless of the result, but it sends messages with the result
unsigned netMessageBodySize(void);
void netListen(void);
void netSend(int flag, const byte* body, unsigned size, unsigned xTo);
void netClean(void);
