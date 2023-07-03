
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*NetMessageReceivedCallback)(unsigned long/*timestamp*/, unsigned/*fromId*/, const byte*/*message*/, unsigned/*size*/);
typedef void (*NetNotifierCallback)(bool); // true on success
typedef void (*NetServiceCallback)(int); // receives message's flag
typedef void (*NetCallback)(void);
typedef unsigned long (*NetCurrentTimeMillisGetter)(void);

struct NetUserInfo_t;
typedef struct NetUserInfo_t NetUserInfo;

typedef void (*NetOnUsersFetched)(NetUserInfo** infos, unsigned size); // receives an array with length of 'size', which is deallocated automatically (and every item inside it) after this callback returns

extern const unsigned NET_USERNAME_SIZE;
extern const unsigned NET_UNHASHED_PASSWORD_SIZE;
extern const unsigned NET_MESSAGE_BODY_SIZE;

bool netInit(
    NetMessageReceivedCallback onMessageReceived,
    NetNotifierCallback onLogInResult,
    NetServiceCallback onErrorReceived, // not called on login error & register error as there are separated callback for them
    NetNotifierCallback onRegisterResult,
    NetCallback onDisconnected, // cleanup is performed after this callback returns, so module needs to be reinitialized after this callback ends to continue working with this module
    NetCurrentTimeMillisGetter currentTimeMillisGetter,
    NetOnUsersFetched onUsersFetched
); // returns true on success

void netLogIn(const char* username, const char* password); // in case of failure the server disconnects client
void netRegister(const char* username, const char* password); // the server disconnects client regardless of the result, but it sends messages with the result
void netListen(void);
unsigned netCurrentUserId(void);
void netSend(int flag, const byte* body, unsigned size, unsigned xTo);
void netShutdownServer(void);
void netFetchUsers(void);
unsigned netUserInfoId(const NetUserInfo* info);
bool netUserInfoConnected(const NetUserInfo* info);
const byte* netUserInfoName(const NetUserInfo* info);
void netClean(void);
