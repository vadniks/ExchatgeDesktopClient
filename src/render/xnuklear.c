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

///////////////////////////////////////////////////////////////////////////////
// https://github.com/Immediate-Mode-UI/Nuklear/blob/master/demo/common/style.c

void nk_set_light_theme(struct nk_context* context) {
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
    table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
    nk_style_from_table(context, table);
}

void nk_set_dark_theme(struct nk_context* context) {
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
    table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
    table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
    table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
    nk_style_from_table(context, table);
}

//
///////////////////////////////////////////////////////////////////////////////
