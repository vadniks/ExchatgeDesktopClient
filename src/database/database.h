
#pragma once

#include <stdbool.h>
#include "../defs.h"
#include "../collections/list.h"

struct DatabaseMessage_t;
typedef struct DatabaseMessage_t DatabaseMessage;

DatabaseMessage* databaseMessageCreate(unsigned* nullable userId, unsigned long timestamp, const char* text, unsigned size); // userId may be null if the message came from current user, 0 < size <= NET_MESSAGE_BODY_SIZE
void databaseMessageDestroy(DatabaseMessage* message);

bool databaseInit(byte* passwordBuffer, unsigned size); // returns true on success (either true or false returned, cleanup is needed to be performed anyway), 'size'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseAddUser(unsigned id, const byte* streamsStates); // returns true on success
bool databaseAddMessage(const DatabaseMessage* message); // returns true on success
List* nullable databaseGetMessages(unsigned userId, unsigned* size); // returns an array of messages <DatabaseMessage*> (which is expected to be deallocated by the caller) on success; size is pointer to buffer in order to obtain array's size
void databaseClean(void);
