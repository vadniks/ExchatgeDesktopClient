
#pragma once

#include <sdl/SDL.h>
#include <stdbool.h>
#include "../defs.h"

typedef void (*RenderCredentialsReceivedCallback)(
    const char* username,
    const char* password,
    bool logIn // true if called from log in page, false if called from register page
);

typedef void (*RenderCredentialsRandomFiller)(char* credentials, unsigned size); // Function that fills credentials fields with random data & doesn't deallocates them
typedef void (*RenderLogInRegisterPageQueriedByUserCallback)(bool logIn);

extern const unsigned RENDER_MAX_ERROR_TEXT_SIZE;

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    RenderCredentialsReceivedCallback onCredentialsReceived,
    RenderCredentialsRandomFiller credentialsRandomFiller,
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser
);

void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);
void renderShowLogIn(void);
void renderShowRegister(void);
void renderShowUsersList(void);
void renderShowError(const char* error); // expects a null-terminated string which size is in range (0, MAX_ERROR_TEXT_SIZE] (with null-terminator);
void renderHideError(void);
void renderDraw(void);
void renderClean(void);
