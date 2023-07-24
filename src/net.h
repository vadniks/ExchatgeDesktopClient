/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include "crypto.h"
#include "defs.h"

typedef void (*NetMessageReceivedCallback)(unsigned long/*timestamp*/, unsigned/*fromId*/, const byte*/*message*/, unsigned/*size*/);
typedef void (*NetNotifierCallback)(bool); // true on success
typedef void (*NetServiceCallback)(int); // receives message's flag
typedef void (*NetCallback)(void);
typedef unsigned long (*NetCurrentTimeMillisGetter)(void);
typedef void (*NetOnConversationSetUpInviteReceived)(unsigned/*fromId*/); // must call replyToPendingConversationSetUpInvite() after this
typedef void (*NetOnFileExchangeInviteReceived)(unsigned fromId); // must then call replyToFileExchangeInvite
typedef bool (*NetNextFileChunkSupplier)(unsigned index, byte* buffer); // returns true if no more chunks available (current chunk included), if this is first time this callback is called, the false return is treated as occurrence of error and the operation gets aborted; copies the another chunk's bytes into the buffer

struct NetUserInfo_t;
typedef struct NetUserInfo_t NetUserInfo;

typedef void (*NetOnUsersFetched)(NetUserInfo** infos, unsigned size); // receives an array with length of 'size', which is deallocated automatically (and every item inside it) after this callback returns

extern const unsigned NET_USERNAME_SIZE;
extern const unsigned NET_UNHASHED_PASSWORD_SIZE;
extern const unsigned NET_MESSAGE_BODY_SIZE;

extern const int NET_FLAG_PROCEED; // just send message to another user

bool netInit( // blocks the caller thread until secure connection is established
    NetMessageReceivedCallback onMessageReceived,
    NetNotifierCallback onLogInResult,
    NetServiceCallback onErrorReceived, // not called on login error & register error as there are separated callback for them
    NetNotifierCallback onRegisterResult,
    NetCallback onDisconnected, // cleanup is performed after this callback returns, so module needs to be reinitialized after this callback ends to continue working with this module
    NetCurrentTimeMillisGetter currentTimeMillisGetter,
    NetOnUsersFetched onUsersFetched,
    NetOnConversationSetUpInviteReceived onConversationSetUpInviteReceived,
    NetOnFileExchangeInviteReceived onFileExchangeInviteReceived,
    NetNextFileChunkSupplier nextFileChunkSupplier
); // returns true on success

void netLogIn(const char* username, const char* password); // in case of failure the server disconnects client
void netRegister(const char* username, const char* password); // the server disconnects client regardless of the result, but it sends messages with the result
void netListen(void);
unsigned netCurrentUserId(void);
bool netSend(int flag, const byte* body, unsigned size, unsigned xTo); // blocks the caller thread, returns true on success; flag is for internal use only, outside the module flag must be FLAG_PROCEED // TODO: hide original function
void netShutdownServer(void);
void netFetchUsers(void);
unsigned netUserInfoId(const NetUserInfo* info);
bool netUserInfoConnected(const NetUserInfo* info);
const byte* netUserInfoName(const NetUserInfo* info);
Crypto* nullable netCreateConversation(unsigned id); // returns the Crypto object associated with newly created conversation on success, expects the id of the user, the current user wanna create conversation with; blocks the caller thread until either a denial received or creation of the conversation succeeds (if an acceptation received) or fails
Crypto* nullable netReplyToPendingConversationSetUpInvite(bool accept, unsigned fromId); // returns the same as createConversation does, must be called after getting invoked by the onConversationSetUpInviteReceived callback to reply to inviter, returns true on success; blocks the caller thread just like createConversation does
bool netBeginFileExchange(unsigned toId); // returns true if another user (identified by toId) accepted the invite
bool netReplyToFileExchangeInvite(unsigned fromId, bool accept); // returns true on success; must be called only after an invite from this user received & processed
void netClean(void);
