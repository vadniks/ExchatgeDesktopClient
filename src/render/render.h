
#pragma once

#include <sdl/SDL.h>
#include <stdbool.h>

typedef void (*CredentialsReceivedCallback)(
    const char* username,
    const char* password,
    bool logIn // true if called from log in page, false if called from register page
);

typedef void (*CredentialsRandomFiller)(char* credentials); // Function that fills credentials fields with random data & doesn't deallocates them

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    CredentialsReceivedCallback onCredentialsReceived,
    CredentialsRandomFiller credentialsRandomFiller
);

void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);
void renderShowLogIn(void);
void renderShowRegister(void);
void renderShowUsersList(void);
void renderDraw(void);
void renderClean(void);
