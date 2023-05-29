
#pragma once

#include <sdl/SDL.h>

void rdInit();
void rdInputBegan();
void rdProcessEvent(SDL_Event* event);
void rdInputEnded();
void rdDraw();
void rdClean();
