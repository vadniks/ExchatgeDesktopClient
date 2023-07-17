
#pragma once

#include <stdbool.h>
#include "../crypto.h"
#include "../defs.h"
#include "../conversationMessage.h"
#include "../collections/list.h"

bool databaseInit(byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize); // returns true on success (either true or false returned, cleanup is needed to be performed anyway), 'passwordSize'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseConversationExists(unsigned userId);
bool databaseAddConversation(unsigned userId, const Crypto* crypto);
bool databaseAddMessage(const unsigned* nullable userId, const ConversationMessage* message); // userId may be null if the message came from the current user
bool databaseUserExists(unsigned id);
List* nullable databaseGetMessages(unsigned userId, unsigned* size); // returns an array of messages <ConversationMessage*> (which is expected to be deallocated by the caller) on success; size is pointer to buffer in order to obtain array's size
void databaseClean(void);
