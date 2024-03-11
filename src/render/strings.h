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

#define TITLE stringsString(0)
#define SUBTITLE stringsString(1)
#define LOG_IN stringsString(2)
#define REGISTER stringsString(3)
#define USERNAME stringsString(4)
#define PASSWORD stringsString(5)
#define PROCEED stringsString(6)
#define USERS_LIST stringsString(7)
#define START_CONVERSATION stringsString(8)
#define CONTINUE_CONVERSATION stringsString(9)
#define DELETE_CONVERSATION stringsString(10)
#define ID_TEXT stringsString(11)
#define NAME_TEXT stringsString(12)
#define ERROR_TEXT stringsString(13)
#define WELCOME stringsString(14)
#define SHUTDOWN_SERVER stringsString(15)
#define DISCONNECTED stringsString(16)
#define UNABLE_TO_CONNECT_TO_THE_SERVER stringsString(17)
#define SEND stringsString(18)
#define ONLINE stringsString(19)
#define OFFLINE stringsString(20)
#define BACK stringsString(21)
#define YOU stringsString(22)
#define UPDATE stringsString(23)
#define REGISTRATION_SUCCEEDED stringsString(24)
#define USER_IS_OFFLINE stringsString(25)
#define INVITATION_RECEIVED stringsString(26)
#define YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER stringsString(27)
#define ACCEPT stringsString(28)
#define DECLINE stringsString(29)
#define UNABLE_TO_DECRYPT_DATABASE stringsString(30)
#define UNABLE_TO_CREATE_CONVERSATION stringsString(31)
#define CONVERSATION_DOESNT_EXIST stringsString(32)
#define CONVERSATION_ALREADY_EXISTS stringsString(33)
#define FILE_EXCHANGE_REQUESTED stringsString(34)
#define FILE_EXCHANGE_REQUESTED_BY_USER stringsString(35)
#define WITH_SIZE_OF stringsString(36)
#define BYTES stringsString(37)
#define CHOOSE stringsString(38)
#define FILE_TEXT stringsString(39)
#define FILE_SELECTION stringsString(40)
#define CANNOT_OPEN_FILE stringsString(41)
#define EMPTY_FILE_PATH stringsString(42)
#define FILE_IS_EMPTY stringsString(43)
#define UNABLE_TO_TRANSMIT_FILE stringsString(44)
#define FILE_IS_TOO_BIG stringsString(45)
#define FILE_TRANSMITTED stringsString(46)
#define ENTER_ABSOLUTE_PATH_TO_FILE stringsString(47)
#define PASTE_WITH_CTRL_V stringsString(48)
#define AUTO_LOGGING_IN stringsString(49)
#define ADMIN_ACTIONS stringsString(50)
#define BROADCAST_MESSAGE stringsString(51)
#define BROADCAST_HINT stringsString(52)
