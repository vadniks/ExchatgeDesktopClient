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

typedef enum : unsigned {
    STRINGS_TITLE = 0,
    STRINGS_SUBTITLE = 1,
    STRINGS_LOG_IN = 2,
    STRINGS_REGISTER = 3,
    STRINGS_USERNAME = 4,
    STRINGS_PASSWORD = 5,
    STRINGS_PROCEED = 6,
    STRINGS_USERS_LIST = 7,
    STRINGS_START_CONVERSATION = 8,
    STRINGS_CONTINUE_CONVERSATION = 9,
    STRINGS_DELETE_CONVERSATION = 10,
    STRINGS_ID_TEXT = 11,
    STRINGS_NAME_TEXT = 12,
    STRINGS_ERROR_TEXT = 13,
    STRINGS_WELCOME = 14,
    STRINGS_SHUTDOWN_SERVER = 15,
    STRINGS_DISCONNECTED = 16,
    STRINGS_UNABLE_TO_CONNECT_TO_THE_SERVER = 17,
    STRINGS_SEND = 18,
    STRINGS_ONLINE = 19,
    STRINGS_OFFLINE = 20,
    STRINGS_BACK = 21,
    STRINGS_YOU = 22,
    STRINGS_UPDATE = 23,
    STRINGS_REGISTRATION_SUCCEEDED = 24,
    STRINGS_USER_IS_OFFLINE = 25,
    STRINGS_INVITATION_RECEIVED = 26,
    STRINGS_YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER = 27,
    STRINGS_ACCEPT = 28,
    STRINGS_DECLINE = 29,
    STRINGS_UNABLE_TO_DECRYPT_DATABASE = 30,
    STRINGS_UNABLE_TO_CREATE_CONVERSATION = 31,
    STRINGS_CONVERSATION_DOESNT_EXIST = 32,
    STRINGS_CONVERSATION_ALREADY_EXISTS = 33,
    STRINGS_FILE_EXCHANGE_REQUESTED = 34,
    STRINGS_FILE_EXCHANGE_REQUESTED_BY_USER = 35,
    STRINGS_WITH_SIZE_OF = 36,
    STRINGS_BYTES = 37,
    STRINGS_CHOOSE = 38,
    STRINGS_FILE_TEXT = 39,
    STRINGS_FILE_SELECTION = 40,
    STRINGS_CANNOT_OPEN_FILE = 41,
    STRINGS_EMPTY_FILE_PATH = 42,
    STRINGS_FILE_IS_EMPTY = 43,
    STRINGS_UNABLE_TO_TRANSMIT_FILE = 44,
    STRINGS_FILE_IS_TOO_BIG = 45,
    STRINGS_FILE_TRANSMITTED = 46,
    STRINGS_ENTER_ABSOLUTE_PATH_TO_FILE = 47,
    STRINGS_PASTE_WITH_CTRL_V = 48,
    STRINGS_AUTO_LOGGING_IN = 49,
    STRINGS_ADMIN_ACTIONS = 50,
    STRINGS_BROADCAST_MESSAGE = 51,
    STRINGS_BROADCAST_HINT = 52
} Strings;
