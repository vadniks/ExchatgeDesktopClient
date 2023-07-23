
#pragma once

#include <stdbool.h>
#include "../crypto.h"
#include "../defs.h"
#include "../collections/list.h"

struct DatabaseMessage_t;
typedef struct DatabaseMessage_t DatabaseMessage;

DatabaseMessage* databaseMessageCreate(unsigned long timestamp, unsigned conversation, unsigned from, const byte* text, unsigned size); // conversation - id of the user; from (user id), the name of the sender otherwise; size of text, whereas size of from is known to all users of this api
unsigned long databaseMessageTimestamp(const DatabaseMessage* message);
unsigned databaseMessageConversation(const DatabaseMessage* message);
unsigned databaseMessageFrom(const DatabaseMessage* message);
const byte* databaseMessageText(const DatabaseMessage* message);
unsigned databaseMessageSize(const DatabaseMessage* message);
void databaseMessageDestroy(DatabaseMessage* message);

bool databaseInit(const byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize, unsigned maxMessageTextSize); // returns true on success (either true or false returned, cleanup is needed to be performed anyway), 'passwordSize'-sized buffer
bool databaseConversationExists(unsigned userId);
bool databaseAddConversation(unsigned userId, const Crypto* crypto); // the user of this api must check for existence of the entity being added before actually adding that entity, otherwise the function's gonna throw... ...smth that the caller surely doesn't want to be thrown into him
Crypto* nullable databaseGetConversation(unsigned userId);
void databaseRemoveConversation(unsigned userId); // check for existence first
bool databaseAddMessage(const DatabaseMessage* message); // check for existence first; from (user id) inside the message may be null if the message came from the current user
List* nullable databaseGetMessages(unsigned conversation); // returns an array of messages <DatabaseMessage*> (which is expected to be deallocated by the caller) on success; inside each message there's fromId unsigned int in bytes (sizeof 4)
void databaseClean(void);
