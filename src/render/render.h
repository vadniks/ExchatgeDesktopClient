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

#include <SDL.h>
#include <stdbool.h>
#include "../collections/list.h"
#include "../defs.h"
#include "strings.h"

typedef void (*RenderCredentialsReceivedCallback)( // buffer is filled with random bytes after callback returns
    const char* username,
    unsigned usernameSize,
    const char* password,
    unsigned passwordSize,
    bool logIn // true if called from log in page, false if called from register page
);

typedef enum {
    RENDER_CONVERSATION_DELETE = -1,
    RENDER_CONVERSATION_START = false, // 0
    RENDER_CONVERSATION_CONTINUE = true // 1
} RenderConversationChooseVariants;

typedef void (*RenderCredentialsRandomFiller)(char* credentials, unsigned size); // Function that fills credentials fields with random data & doesn't deallocates them
typedef void (*RenderLogInRegisterPageQueriedByUserCallback)(bool logIn);
typedef void (*RenderOnServerShutdownRequested)(void);
typedef void (*RenderOnReturnFromConversationPageRequested)(void);
typedef char* (*RenderMillisToDateTimeConverter)(unsigned long); // returns null-terminated formatted string with date & time that must be deallocated by the caller
typedef void (*RenderOnSendClicked)(const char* text, unsigned size); // receives an auto deallocated text of the message the user wanna send, text length is equal to size which is in range (0, this->maxMessageSize]
typedef void (*RenderOnUpdateUsersListClicked)(void);
typedef void (*RenderOnFileChooserRequested)(void);
typedef void (*RenderFileChooseResultHandler)(const char* nullable filePath, unsigned size); // receives absolute path of the chosen file (which is deallocated automatically and therefore must be copied), or null and zero size if no file was chosen (return requested) or error occurred
typedef void (*RenderOnAutoLoggingInChanged)(bool value);
typedef bool (*RenderAutoLoggingInSupplier)(void);
typedef void (*RenderUserForConversationChosenCallback)(unsigned id, RenderConversationChooseVariants chooseVariant);
typedef void (*RenderOnAdminActionsPageRequested)(bool enter); // true if the page is needed to be shown, else - to be hidden
typedef void (*RenderOnBroadcastMessageSendRequested)(const char* text, unsigned size);
typedef bool (*RenderFilePresenceChecker)(const char* file); // returns true if exists

typedef enum : byte {
    RENDER_THEME_LIGHT = 0,
    RENDER_THEME_DARK = 1
} RenderThemes;

extern const unsigned RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE; // TODO: make inline all one-line functions in implementation

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    RenderCredentialsReceivedCallback onCredentialsReceived,
    RenderCredentialsRandomFiller credentialsRandomFiller,
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser,
    RenderUserForConversationChosenCallback onUserForConversationChosen,
    unsigned conversationNameSize,
    RenderOnServerShutdownRequested onServerShutdownRequested,
    RenderOnReturnFromConversationPageRequested onReturnFromConversationPageRequested,
    RenderMillisToDateTimeConverter millisToDateTimeConverter,
    RenderOnSendClicked onSendClicked,
    RenderOnUpdateUsersListClicked onUpdateUsersListClicked,
    unsigned maxFilePathSize,
    RenderOnFileChooserRequested onFileChooserRequested,
    RenderFileChooseResultHandler fileChooseResultHandler,
    RenderOnAutoLoggingInChanged onAutoLoggingInChanged,
    RenderAutoLoggingInSupplier autoLoggingInSupplier,
    RenderOnAdminActionsPageRequested onAdminActionsPageRequested,
    RenderOnBroadcastMessageSendRequested onBroadcastMessageSendRequested,
    RenderFilePresenceChecker filePresenceChecker
);

void renderPostInit( // must be called before first call to renderDraw() to initialize things
    unsigned maxMessageSize,
    bool adminMode,
    RenderThemes theme,
    List* usersList, // <User*> must be deallocated by a caller of the renderInit function after work with the module itself is finished (renderClean is called)
    List* messagesList, // <ConversationMessage*> must be deallocated by the caller after this module gets shut down
    StringsLanguages language
);

void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);

void renderSetWindowTitle(const char* title); // expects a this->usernameSize-sized string, functions creates a formatted string like '<WINDOW_TITLE>, <title>' ('Exchatge, user1')
void renderAlterConversationMessageBuffer(const char* text, unsigned size); // only available when conversation is shown
void renderAlterFilePathBuffer(const char* filePath, unsigned size); // only available when file chooser is shown

void renderShowLogIn(void);
void renderShowRegister(void);
void renderShowUsersList(const char* currentUserName); // the name of the user who is currently logged in via this client, this->usernameSize-sized, copied
void renderShowConversation(const char* conversationName); // expects the name (which is copied) (with length == this->conversationNameSize) of the user with whom the current user will have a conversation or the name of that conversation
void renderShowFileChooser(void);
void renderShowAdminActions(void);

bool renderIsConversationShown(void);
bool renderIsFileChooserShown(void);

bool renderShowInviteDialog(const char* fromUserName); // blocks the caller thread, returns true if user accepted the invitation, expects a this->username-sized string - the name of the user who has sent the invitation
bool renderShowFileExchangeRequestDialog(const char* fromUserName, unsigned fileSize, const char* filename); // almost just like showInviteDialog

void renderSetControlsBlocking(bool blocking); // true to block controls, false to unblock, use with show*Dialog

void renderShowSystemMessage(const char* text, unsigned size);
void renderShowSystemError(void); // just shows an error system message with text 'Error'
void renderShowDisconnectedError(void);
void renderShowUnableToConnectToTheServerError(void); // TODO: too long name
void renderShowRegistrationSucceededSystemMessage(void);
void renderShowUserIsOfflineError(void);
void renderShowUnableToDecryptDatabaseError(void);
void renderShowUnableToCreateConversation(void);
void renderShowConversationDoesntExist(void);
void renderShowConversationAlreadyExists(void);
void renderShowCannotOpenFileError(void);
void renderShowEmptyFilePathError(void);
void renderShowFileIsEmptyError(void);
void renderShowUnableToTransmitFileError(void);
void renderShowFileIsTooBig(void);
void renderShowFileTransmittedSystemMessage(void);

void renderShowInfiniteProgressBar(void); // showed only on pages that support it (log in/register, not splash as it's a special case)
void renderHideInfiniteProgressBar(void);

void renderDraw(void);

void renderClean(void);
