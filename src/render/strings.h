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
    TITLE = 0,
    SUBTITLE = 1,
    LOG_IN = 2,
    REGISTER = 3,
    USERNAME = 4,
    PASSWORD = 5,
    PROCEED = 6,
    USERS_LIST = 7,
    START_CONVERSATION = 8,
    CONTINUE_CONVERSATION = 9,
    DELETE_CONVERSATION = 10,
    ID_TEXT = 11,
    NAME_TEXT = 12,
    ERROR_TEXT = 13,
    WELCOME = 14,
    SHUTDOWN_SERVER = 15,
    DISCONNECTED = 16,
    UNABLE_TO_CONNECT_TO_THE_SERVER = 17,
    SEND = 18,
    ONLINE = 19,
    OFFLINE = 20,
    BACK = 21,
    YOU = 22,
    UPDATE = 23,
    REGISTRATION_SUCCEEDED = 24,
    USER_IS_OFFLINE = 25,
    INVITATION_RECEIVED = 26,
    YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER = 27,
    ACCEPT = 28,
    DECLINE = 29,
    UNABLE_TO_DECRYPT_DATABASE = 30,
    UNABLE_TO_CREATE_CONVERSATION = 31,
    CONVERSATION_DOESNT_EXIST = 32,
    CONVERSATION_ALREADY_EXISTS = 33,
    FILE_EXCHANGE_REQUESTED = 34,
    FILE_EXCHANGE_REQUESTED_BY_USER = 35,
    WITH_SIZE_OF = 36,
    BYTES = 37,
    CHOOSE = 38,
    FILE_TEXT = 39,
    FILE_SELECTION = 40,
    CANNOT_OPEN_FILE = 41,
    EMPTY_FILE_PATH = 42,
    FILE_IS_EMPTY = 43,
    UNABLE_TO_TRANSMIT_FILE = 44,
    FILE_IS_TOO_BIG = 45,
    FILE_TRANSMITTED = 46,
    ENTER_ABSOLUTE_PATH_TO_FILE = 47,
    PASTE_WITH_CTRL_V = 48,
    AUTO_LOGGING_IN = 49,
    ADMIN_ACTIONS = 50,
    BROADCAST_MESSAGE = 51,
    BROADCAST_HINT = 52
} Strings;
