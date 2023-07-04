
#pragma once

#include "defs.h"
#include <collections/list.h>

typedef struct {
    unsigned long timestamp;
    char* nullable from; // null if from this client, the name of the sender otherwise
    char* text;
    unsigned size; // size of text, whereas size of from is known to all users of this api
} ConversationMessage;

List* conversationMessageInitList(void); // the list must be deallocated via the listDestroy(...), elements in it will be deallocated automatically by the list's deallocater which is set here
ConversationMessage* conversationMessageCreate(
    unsigned long timestamp,
    const char* nullable from,
    unsigned fromSize,
    const char* text,
    unsigned textSize
); // from: name of the sender (is copied) (is in range (0, NET_USERNAME_SIZE]), null if from this client; text: text (whose length == size and 0 < length <= NET_MESSAGE_BODY_SIZE) is copied
void conversationMessageDestroy(ConversationMessage* message);
