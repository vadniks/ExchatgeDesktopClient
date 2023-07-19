
#pragma once

#include <stdbool.h>
#include "../crypto.h"
#include "../defs.h"
#include "../collections/list.h"

struct DatabaseMessage_t;
typedef struct DatabaseMessage_t DatabaseMessage;

DatabaseMessage* databaseMessageCreate(unsigned long timestamp, const unsigned* nullable from, const byte* text, unsigned size); // from (user id) null if from this client, the name of the sender otherwise; size of text, whereas size of from is known to all users of this api
unsigned long databaseMessageTimestamp(const DatabaseMessage* message);
const unsigned* databaseMessageFrom(const DatabaseMessage* message);
const byte* databaseMessageText(const DatabaseMessage* message);
unsigned databaseMessageSize(const DatabaseMessage* message);
void databaseMessageDestroy(DatabaseMessage* message);

bool databaseInit(byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize, unsigned maxMessageTextSize); // returns true on success (either true or false returned, cleanup is needed to be performed anyway), 'passwordSize'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseConversationExists(unsigned userId);
bool databaseAddConversation(unsigned userId, const Crypto* crypto);
Crypto* nullable databaseGetConversation(unsigned userId);
bool databaseMessageExists(unsigned long timestamp, const unsigned* nullable from);
bool databaseAddMessage(const DatabaseMessage* message); // from (user id) inside the message may be null if the message came from the current user
List* nullable databaseGetMessages(unsigned userId); // returns an array of messages <DatabaseMessage*> (which is expected to be deallocated by the caller) on success; inside each message there's fromId unsigned int in bytes (sizeof 4)
void databaseClean(void);
