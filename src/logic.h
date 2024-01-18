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

#include <stdbool.h>
#include "defs.h"
#include "collections/list.h"

extern const unsigned LOGIC_MAX_FILE_PATH_SIZE;

void logicInit(void);
bool logicIsAdminMode(void);
bool logicIsDarkTheme(void);
void logicNetListen(void); // causes net module to listen for connection updates
List* logicUsersList(void); // returns permanent users list in which actual user objects will be inserted/updated/removed later by the net module
List* logicMessagesList(void); // same as usersList but for conversation messages between users
void logicOnAutoLoggingInChanged(bool value);
bool logicAutoLoggingInSupplier(void);
void logicOnFileChooserRequested(void);
void logicFileChooseResultHandler(const char* nullable filePath, unsigned size);
void logicOnAdminActionsPageRequested(bool enter);
void logicOnBroadcastMessageSendRequested(const char* text, unsigned size);
void logicProcessEvent(SDL_Event* event);
void logicOnCredentialsReceived(const char* username, unsigned usernameSize, const char* password, unsigned passwordSize, bool logIn);
void logicCredentialsRandomFiller(char* credentials, unsigned size);
void logicOnLoginRegisterPageQueriedByUser(bool logIn);
void logicOnUserForConversationChosen(unsigned id, RenderConversationChooseVariants chooseVariant);
void logicOnServerShutdownRequested(void);
void logicOnReturnFromConversationPageRequested(void);
char* logicMillisToDateTime(unsigned long millis); // result is a null-terminated formatted string deallocation of which must be performed by the caller
unsigned long logicCurrentTimeMillis(void);
void logicOnSendClicked(const char* text, unsigned size); // expects a string with 'size' in range (0, NET_MAX_MESSAGE_BODY_SIZE] which is copied
void logicOnUpdateUsersListClicked(void);
unsigned logicMaxMessagePlainPayloadSize(void);
void logicClean(void);
