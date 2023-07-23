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
#include "collections/list.h"

typedef struct {
    unsigned id;
    char* name;
    bool conversationExists; // true if current user (who has logged in via this client) and this user (who displayed in the users list) have already started a conversation
    bool online;
} User;

List* userInitList(void); // the list must be deallocated via the listDestroy(...), elements in it will be deallocated automatically by the list's deallocater which is set here
User* userCreate(
    unsigned id,
    const char* name,
    unsigned nameSize,
    bool conversationExists,
    bool online
); // name (which is null-terminated string with (0, this->usernameSize] range sized length) is copied
void userDestroy(User* user);
