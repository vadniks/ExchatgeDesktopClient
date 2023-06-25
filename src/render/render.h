
#pragma once // TODO: rename this module to UI

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

typedef struct {
    unsigned id;
    char* name;
    bool conversationExists; // true if current user (who has logged in via this client) and this user (who displayed in the users list) have already started a conversation
    void (*onClicked)(unsigned id);
} RenderUser;

typedef struct {
    unsigned long timestamp;
    bool fromThisClient;
    char* text;
    unsigned size;
} RenderMessage;

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
    unsigned conversationMessageSize
);
void renderSetAdminMode(bool mode);

List* renderInitUsersList(void);
RenderUser* renderCreateUser(unsigned id, const char* name, bool conversationExists); // name (which is null-terminated string with (0, this->usernameSize] range sized length) is copied
void renderDestroyUser(RenderUser* user);
void renderSetUsersList(const List* usersList); // <RenderUser*> must be deallocated by a caller of the renderInit function after work with the module itself is finished (renderClean is called)

List* renderInitMessagesList(void);
RenderMessage* renderCreateMessage(unsigned long timestamp, bool fromThisClient, const char* text, unsigned size); // text (whose length == size and 0 < length <= this->maxMessageSize) is copied
void renderDestroyMessage(RenderMessage* message);
void renderSetMessagesList(const List* messagesList);

void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);

void renderShowLogIn(void);
void renderShowRegister(void);
void renderShowUsersList(void);
void renderShowConversation(const char* conversationName); // expects the name (which is copied) (with length == this->conversationNameSize) of the user with whom the current user will have a conversation or the name of that conversation

void renderShowSystemMessage(const char* message, bool error); // shows system text to the user, expects a null-terminated string which size is in range (0, MAX_ERROR_TEXT_SIZE] (with null-terminator included);
void renderHideSystemMessage(void);
void renderShowSystemError(void); // just shows an error system message with text 'Error'
void renderShowDisconnectedSystemMessage(void);
void renderShowUnableToConnectToTheServerSystemMessage(void); // TODO: too long name

void renderShowInfiniteProgressBar(void); // showed only on pages that support it (log in/register, not splash as it's a special case)
void renderHideInfiniteProgressBar(void);

void renderDraw(void);
void renderClean(void);
