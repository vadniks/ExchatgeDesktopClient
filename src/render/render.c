
#include <stdbool.h>
#include <assert.h>
//#include "nuklearDefs.h" - cmake automatically includes precompiled version of this header
#include "../defs.h"
#include "render.h"

static const char* WINDOW_TITLE = "Exchatge";
static const unsigned WINDOW_WIDTH = 16 * 50;
static const unsigned WINDOW_HEIGHT = 9 * 50;
static const unsigned WINDOW_MINIMAL_WIDTH = WINDOW_WIDTH / 2;
static const unsigned WINDOW_MINIMAL_HEIGHT = WINDOW_HEIGHT / 2;

static const unsigned STATE_INITIAL = 0;
static const unsigned STATE_LOG_IN = 1;
static const unsigned STATE_REGISTER = 2;
static const unsigned STATE_USERS_LIST = 3;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    unsigned width;
    unsigned height;
    SDL_Window* window;
    SDL_Renderer* renderer;
    struct nk_context* context;
    struct nk_colorf colorf;
    unsigned state;
    CredentialsReceivedCallback onCredentialsReceived;
)
#pragma clang diagnostic pop

static void setStyle(void) {
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
    nk_style_from_table(this->context, table);
}

void renderInit(void) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->width = WINDOW_WIDTH;
    this->height = WINDOW_HEIGHT;
    this->state = STATE_INITIAL;
    this->onCredentialsReceived = NULL;

    this->window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        (int) this->width,
        (int) this->height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
    );
    assert(this->window);

    SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    this->renderer = SDL_CreateRenderer(this->window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    assert(this->renderer);

    int renderW, renderH, windowW, windowH;
    float scaleX, scaleY, fontScale;

    SDL_GetRendererOutputSize(this->renderer, &renderW, &renderH);
    SDL_GetWindowSize(this->window, &windowW, &windowH);

    scaleX = (float) renderW / (float) windowW;
    scaleY = (float) renderH / (float) windowH;

    SDL_RenderSetScale(this->renderer, scaleX, scaleY);
    fontScale = scaleY;

    this->context = nk_sdl_init(this->window, this->renderer);
    struct nk_font_config config = nk_font_config(0);

    struct nk_font_atlas* atlas = NULL;
    nk_sdl_font_stash_begin(&atlas);

    struct nk_font* font = nk_font_atlas_add_default(atlas, 14 * fontScale, &config);
    nk_sdl_font_stash_end();

    font->handle.height /= fontScale;
    nk_style_set_font(this->context, &(font->handle));

    setStyle();

    this->colorf = (struct nk_colorf) { 0.10f, 0.18f, 0.24f, 1.00f };
}

void renderInputBegan(void) {
    assert(this);
    nk_input_begin(this->context);
}

void renderProcessEvent(SDL_Event* event) {
    assert(this);
    nk_sdl_handle_event(event);
}

void renderInputEnded(void) {
    assert(this);
    nk_input_end(this->context);
}

void renderShowLogIn(CredentialsReceivedCallback onCredentialsReceived) {
    assert(this);
    this->onCredentialsReceived = onCredentialsReceived;
    this->state = STATE_LOG_IN;
}

void renderShowRegister(CredentialsReceivedCallback onCredentialsReceived) {
    assert(this);
    this->onCredentialsReceived = onCredentialsReceived;
    this->state = STATE_REGISTER;
}

void renderShowUsersList(void) {
    assert(this);
    this->state = STATE_USERS_LIST;
}

static void drawLoginPage(bool xRegister) {
    nk_layout_row_dynamic(this->context, 15, 2);
    nk_label(this->context, "Unable to connect", NK_TEXT_CENTERED);

    if (nk_button_label(this->context, "Retry")) {}

    nk_spacer(this->context);
}

static void drawPage(void) {
    switch (this->state) {
        case STATE_INITIAL:
            // TODO: splash screen
            break;
        case STATE_LOG_IN:
            drawLoginPage(false);
            break;
        case STATE_REGISTER:
            drawLoginPage(true);
            break;
        case STATE_USERS_LIST:

            break;
        default: assert(false);
    }
}

static void drawWindowTooLittleStub(void) { // 'cause SDL_SetWindowMinimumSize doesn't prevent user from resizing below minimum bounds
    byte r, g, b, a;
    SDL_GetRenderDrawColor(this->renderer, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(this->renderer, 0xff, 0, 0, 0xff);

    SDL_Rect rect = { 0, 0, (int) this->width, (int) this->height };
    SDL_RenderDrawRect(this->renderer, &rect);

    SDL_RenderDrawLine(this->renderer, 0, 0, (int) this->width, (int) this->height);
    SDL_RenderDrawLine(this->renderer, 0, (int) this->height, (int) this->width, 0);

    SDL_SetRenderDrawColor(this->renderer, r, g, b, a);
}

void renderDraw(void) {
    SDL_SetRenderDrawColor(
        this->renderer,
        (int) this->colorf.r * 255,
        (int) this->colorf.g * 255,
        (int) this->colorf.b * 255,
        (int) this->colorf.a * 255
    );
    SDL_RenderClear(this->renderer);

    SDL_GetWindowSizeInPixels(this->window, (int*) &(this->width), (int*) &(this->height));
    if (this->width >= WINDOW_MINIMAL_WIDTH && this->height >= WINDOW_MINIMAL_HEIGHT) {

        if (nk_begin(
            this->context,
            "Exchatge",
            nk_rect(0, 0, (float) this->width, (float) this->height),
            NK_WINDOW_BORDER
        )) drawPage();

        nk_end(this->context);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
    } else
        drawWindowTooLittleStub();

    SDL_RenderPresent(this->renderer);
}

void renderClean(void) {
    if (!this) return;

    nk_sdl_shutdown();
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);

    SDL_free(this);
    this = NULL;
}
