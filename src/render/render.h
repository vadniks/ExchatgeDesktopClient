
#pragma once

#include <sdl/SDL.h>
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

typedef struct {
    unsigned id;
    char* name;
    void (*onClicked)(unsigned id);
} RenderUser;

extern const unsigned RENDER_MAX_MESSAGE_TEXT_SIZE;

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    RenderCredentialsReceivedCallback onCredentialsReceived,
    RenderCredentialsRandomFiller credentialsRandomFiller,
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser
);

List* renderInitUsersList(void);
RenderUser* renderCreateUser(unsigned id, const char* name); // name (which is null-terminated string with (0, this->usernameSize] range sized length) is copied
void renderDestroyUser(RenderUser* user);
void renderSetUsersList(const List* usersList); // <RenderUser*> must be deallocated by a caller of the renderInit function after work with the module itself is finished (renderClean is called)
void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);
void renderShowLogIn(void);
void renderShowRegister(void);
void renderShowUsersList(void);
void renderShowMessageExchange(void);
void renderShowMessage(const char* message, bool error); // shows system message to the user, expects a null-terminated string which size is in range (0, MAX_ERROR_TEXT_SIZE] (with null-terminator included);
void renderHideMessage(void);
void renderDraw(void);
void renderClean(void);
