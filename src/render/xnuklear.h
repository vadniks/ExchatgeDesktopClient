/** Proxy header, prototypes for the Nuklear library */

#pragma once

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define INCLUDE_STYLE

#pragma clang diagnostic pop

#include <nuklear.h>
#include <nuklear_sdl_renderer.h>

#ifndef STBTT_MAX_OVERSAMPLE
#   define STBTT_MAX_OVERSAMPLE 8
#endif
