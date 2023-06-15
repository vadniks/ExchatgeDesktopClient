
#pragma once

#include <sdl/SDL.h>

void renderInit(void);
void renderInputBegan(void);
void renderProcessEvent(SDL_Event* event);
void renderInputEnded(void);
void renderDraw(void);
void renderClean(void);
