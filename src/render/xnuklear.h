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

extern const nk_rune nk_font_glyph_ranges[];

nk_flags nk_edit_password_string(
    struct nk_context* ctx,
    nk_flags flags,
    char* memory,
    int* len,
    int max,
    nk_plugin_filter filter
);
