
#pragma once

#include <stdbool.h>
#include "../defs.h"

#pragma pack(true)

typedef struct {
    unsigned* nullable userId; // userId may be null if the message came from current user, 0 < size <= NET_MESSAGE_BODY_SIZE
    unsigned long timestamp;
    char* text; // needs to be copied
    unsigned size;
} DatabaseMessage;

bool databaseInit(byte* passwordBuffer, unsigned size); // returns true on success (either true or false cleanup is needed to be performed anyway), 'size'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseAddUser(unsigned id); // returns true on success
bool databaseAddMessage(const DatabaseMessage* message); // returns true on success
DatabaseMessage** nullable databaseGetMessages(unsigned userId, unsigned* size); // returns an array of messages (which is expected to be deallocated by the caller) on success; size is pointer to buffer in order to obtain array's size
void databaseClean(void);
