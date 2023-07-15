
#pragma once

#include <stdbool.h>
#include "../defs.h"

#pragma pack(true)

struct Message_t;
typedef struct Message_t Message;

bool databaseInit(byte* passwordBuffer, unsigned size); // returns true on success (either true or false cleanup is needed to be performed anyway), 'size'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseAddUser(unsigned id); // returns true on success
Message* databaseCreateMessage(unsigned* nullable userId, unsigned long timestamp, const char* text, unsigned size); // userId may be null if the message came from current user, 0 < size <= NET_MESSAGE_BODY_SIZE
void databaseDestroyMessage(Message* message);
bool databaseAddMessage(const Message* message); // returns true on success
Message** nullable databaseGetMessages(unsigned userId, unsigned* size); // returns an array of messages (which is expected to be deallocated by the caller) on success; size is pointer to buffer in order to obtain array's size
void databaseClean(void);
