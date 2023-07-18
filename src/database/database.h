
#pragma once

#include <stdbool.h>
#include "../crypto.h"
#include "../defs.h"
#include "../conversationMessage.h"
#include "../collections/list.h"

bool databaseInit(byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize, unsigned maxMessageTextSize); // returns true on success (either true or false returned, cleanup is needed to be performed anyway), 'passwordSize'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseConversationExists(unsigned userId);
bool databaseAddConversation(unsigned userId, const Crypto* crypto);
bool databaseMessageExists(unsigned long timestamp, const unsigned* nullable from);
bool databaseAddMessage(const unsigned* nullable fromUserId, const ConversationMessage* message); // fromUserId may be null if the message came from the current user
List* nullable databaseGetMessages(unsigned userId, unsigned* size); // returns an array of messages <ConversationMessage*> (which is expected to be deallocated by the caller) on success; inside each message there's fromId unsigned int in bytes (sizeof 4); size is pointer to buffer in order to obtain array's size
void databaseClean(void);
