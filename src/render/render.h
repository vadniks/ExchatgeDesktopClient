
#pragma once

#include <sdl/SDL.h> // TODO: store messages (encrypted) on the client side
#include <stdbool.h>
#include "collections/list.h"
#include "../defs.h"

typedef void (*RenderCredentialsReceivedCallback)( // buffer is filled with random bytes after callback returns
    const char* username,
    const char* password,
    bool logIn // true if called from log in page, false if called from register page
);

typedef void (*RenderCredentialsRandomFiller)(char* credentials, unsigned size); // Function that fills credentials fields with random data & doesn't deallocates them
typedef void (*RenderLogInRegisterPageQueriedByUserCallback)(bool logIn);
typedef void (*RenderOnServerShutdownRequested)(void);
typedef void (*RenderOnReturnFromConversationPageRequested)(void);
typedef char* (*RenderMillisToDateTimeConverter)(unsigned long); // returns null-terminated formatted string with date & time that must be deallocated by the caller
typedef void (*RenderOnSendClicked)(const char* text, unsigned size); // receives an auto deallocated text of the message the user wanna send, text length is equal to size which is in range (0, this->maxMessageSize]
typedef void (*RenderOnUpdateUsersListClicked)(void);

typedef enum {
    RENDER_DELETE_CONVERSATION = -1,
    RENDER_START_CONVERSATION = false, // 0
    RENDER_CONTINUE_CONVERSATION = true // 1
} RenderConversationChooseVariants;

typedef void (*RenderUserForConversationChosenCallback)(unsigned id, RenderConversationChooseVariants chooseVariant);

extern const unsigned RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE;

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    RenderCredentialsReceivedCallback onCredentialsReceived,
    RenderCredentialsRandomFiller credentialsRandomFiller,
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser,
    RenderUserForConversationChosenCallback onUserForConversationChosen,
    unsigned maxMessageSize,
    unsigned conversationNameSize,
    RenderOnServerShutdownRequested onServerShutdownRequested,
    RenderOnReturnFromConversationPageRequested onReturnFromConversationPageRequested,
    RenderMillisToDateTimeConverter millisToDateTimeConverter,
    RenderOnSendClicked onSendClicked,
    RenderOnUpdateUsersListClicked onUpdateUsersListClicked
);

void renderSetAdminMode(bool mode);
void renderSetUsersList(const List* usersList); // <User*> must be deallocated by a caller of the renderInit function after work with the module itself is finished (renderClean is called)
void renderSetMessagesList(const List* messagesList); // <ConversationMessage*> must be deallocated by the caller after this module gets shut down

void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);

void renderSetWindowTitle(const char* title); // expects a this->usernameSize-sized string, functions creates a formatted string like '<WINDOW_TITLE>, <title>' ('Exchatge, user1')

void renderShowLogIn(void);
void renderShowRegister(void);
void renderShowUsersList(const char* currentUserName); // the name of the user who is currently logged in via this client, this->usernameSize-sized, copied
void renderShowConversation(const char* conversationName); // expects the name (which is copied) (with length == this->conversationNameSize) of the user with whom the current user will have a conversation or the name of that conversation

bool renderShowInviteDialog(const char* fromUserName); // blocks the caller thread, returns true if user accepted the invitation, expects a this->username-sized string - the name of the user who has sent the invitation
void renderSetControlsBlocking(bool blocking); // true to block controls, false to unblock, use with show*Dialog

void renderShowSystemMessage(const char* message, bool error); // shows system text to the user, expects a null-terminated string which size is in range (0, MAX_ERROR_TEXT_SIZE] (with null-terminator included);
void renderHideSystemMessage(void);
void renderShowSystemError(void); // just shows an error system message with text 'Error'
void renderShowDisconnectedError(void);
void renderShowUnableToConnectToTheServerError(void); // TODO: too long name
void renderShowRegistrationSucceededSystemMessage(void);
void renderShowUserIsOfflineError(void);

void renderShowInfiniteProgressBar(void); // showed only on pages that support it (log in/register, not splash as it's a special case)
void renderHideInfiniteProgressBar(void);

void renderDraw(void);
void renderClean(void);
