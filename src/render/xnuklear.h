/** Proxy header, prototypes for the Nuklear library */

#pragma once

#include "xnuklearDefs.h"
#include <nuklear.h>
#include <nuklear_sdl_renderer.h>

#ifndef STBTT_MAX_OVERSAMPLE
#   define STBTT_MAX_OVERSAMPLE 8
#endif

void nk_set_light_theme(struct nk_context* context);
void nk_set_dark_theme(struct nk_context* context);
