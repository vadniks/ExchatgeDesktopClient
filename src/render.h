
#pragma once

#include <sdl/SDL.h>

void renderInit();
void renderInputBegan();
void renderProcessEvent(SDL_Event* event);
void renderInputEnded();
void renderDraw();
void renderClean();
