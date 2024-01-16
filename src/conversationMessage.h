/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)
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

#include "defs.h"
#include "collections/list.h"

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
); // from: name of the sender (is copied) (is in range (0, NET_USERNAME_SIZE]), null if from this client; text: text (whose length == size and 0 < length <= NET_MAX_MESSAGE_BODY_SIZE) is copied
void conversationMessageDestroy(ConversationMessage* message);
