/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
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
