
#pragma once

#include <stdbool.h>
#include "collections/list.h"

typedef struct {
    unsigned id;
    char* name;
    bool conversationExists; // true if current user (who has logged in via this client) and this user (who displayed in the users list) have already started a conversation
    bool online;
} User;

List* userInitList(void);
User* userCreate(
    unsigned id,
    const char* name,
    unsigned nameSize,
    bool conversationExists,
    bool online
); // name (which is null-terminated string with (0, this->usernameSize] range sized length) is copied
void userDestroy(User* user);
