/** The actual implementation of the Nuklear library */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION

#pragma clang diagnostic pop

////////////////////////////////////////////////////////

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything" // external read-only code inclusion

#include "xnuklearDefs.h"
#include <nuklear.h>
#include <nuklear_sdl_renderer.h>

#pragma clang diagnostic pop
