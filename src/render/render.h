
#pragma once

#include <sdl/SDL.h>

typedef void (*CredentialsReceivedCallback)(
    const char* username,
    const char* password,
    unsigned usernameSize,
    unsigned passwordSize
);

void renderInit(void);
void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);
void renderShowLogIn(CredentialsReceivedCallback onCredentialsReceived);
void renderShowRegister(CredentialsReceivedCallback onCredentialsReceived);
void renderShowUsersList(void);
void renderDraw(void);
void renderClean(void);
