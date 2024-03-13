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

#include "../defs.h"

typedef enum : byte {
    STRINGS_LANGUAGE_ENGLISH = 0,
    STRINGS_LANGUAGE_RUSSIAN = 1
} StringsLanguages;

extern const unsigned STRINGS;

void stringsSetLanguage(StringsLanguages language);
const char* stringsString(unsigned id);

extern const unsigned TITLE;
extern const unsigned SUBTITLE;
extern const unsigned LOG_IN;
extern const unsigned REGISTER;
extern const unsigned USERNAME;
extern const unsigned PASSWORD;
extern const unsigned PROCEED;
extern const unsigned USERS_LIST;
extern const unsigned START_CONVERSATION;
extern const unsigned CONTINUE_CONVERSATION;
extern const unsigned DELETE_CONVERSATION;
extern const unsigned ID_TEXT;
extern const unsigned NAME_TEXT;
extern const unsigned ERROR_TEXT;
extern const unsigned WELCOME;
extern const unsigned SHUTDOWN_SERVER;
extern const unsigned DISCONNECTED;
extern const unsigned UNABLE_TO_CONNECT_TO_THE_SERVER;
extern const unsigned SEND;
extern const unsigned ONLINE;
extern const unsigned OFFLINE;
extern const unsigned BACK;
extern const unsigned YOU;
extern const unsigned UPDATE;
extern const unsigned REGISTRATION_SUCCEEDED;
extern const unsigned USER_IS_OFFLINE;
extern const unsigned INVITATION_RECEIVED;
extern const unsigned YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER;
extern const unsigned ACCEPT;
extern const unsigned DECLINE;
extern const unsigned UNABLE_TO_DECRYPT_DATABASE;
extern const unsigned UNABLE_TO_CREATE_CONVERSATION;
extern const unsigned CONVERSATION_DOESNT_EXIST;
extern const unsigned CONVERSATION_ALREADY_EXISTS;
extern const unsigned FILE_EXCHANGE_REQUESTED;
extern const unsigned FILE_EXCHANGE_REQUESTED_BY_USER;
extern const unsigned WITH_SIZE_OF;
extern const unsigned BYTES;
extern const unsigned CHOOSE;
extern const unsigned FILE_TEXT;
extern const unsigned FILE_SELECTION;
extern const unsigned CANNOT_OPEN_FILE;
extern const unsigned EMPTY_FILE_PATH;
extern const unsigned FILE_IS_EMPTY;
extern const unsigned UNABLE_TO_TRANSMIT_FILE;
extern const unsigned FILE_IS_TOO_BIG;
extern const unsigned FILE_TRANSMITTED;
extern const unsigned ENTER_ABSOLUTE_PATH_TO_FILE;
extern const unsigned PASTE_WITH_CTRL_V;
extern const unsigned AUTO_LOGGING_IN;
extern const unsigned ADMIN_ACTIONS;
extern const unsigned BROADCAST_MESSAGE;
extern const unsigned BROADCAST_HINT;
