
#include <sdl/SDL.h>
#include <sdl_net/SDL_net.h>
#include <stdbool.h>
#include "nuklear.h"

static void setStyle(struct nk_context* context) {
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

static char* net() {
    IPaddress address;
    SDLNet_ResolveHost(&address, "127.0.0.1", 8080);

    char* buffer = NULL;

    TCPsocket socket = SDLNet_TCP_OpenClient(&address);
    if (!socket) goto end;

    SDLNet_TCP_Send(socket, "Hello", 5);

    buffer = SDL_calloc(256, sizeof(char));
    SDLNet_TCP_Recv(socket, buffer, 256);
    SDL_Log("%s\n", buffer);

    end:
    SDLNet_TCP_Close(socket);
    return buffer;
}

int main() {
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
    SDLNet_Init();

    const int width = 500, height = 500;
    SDL_Window* window = SDL_CreateWindow(
        "Exchatge",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    int renderW, renderH, windowW, windowH;
    float scaleX, scaleY, fontScale;

    SDL_GetRendererOutputSize(renderer, &renderW, &renderH);
    SDL_GetWindowSize(window, &windowW, &windowH);

    scaleX = (float) renderW / (float) windowW;
    scaleY = (float) renderH / (float) windowH;

    SDL_RenderSetScale(renderer, scaleX, scaleY);
    fontScale = scaleY;

    // nuklear

    struct nk_context* context = nk_sdl_init(window, renderer);
    struct nk_font_config config = nk_font_config(0);

    struct nk_font_atlas* atlas = NULL;
    nk_sdl_font_stash_begin(&atlas);

    struct nk_font* font = nk_font_atlas_add_default(atlas, 14 * fontScale, &config);
    nk_sdl_font_stash_end();

    font->handle.height /= fontScale;
    nk_style_set_font(context, &(font->handle));

    setStyle(context);

    struct nk_colorf colorf = { 0.10f, 0.18f, 0.24f, 1.00f };

    // net

    char* message = net();

    // loop

    SDL_Event event;
    while (true) {
        nk_input_begin(context);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) goto cleanup;
            nk_sdl_handle_event(&event);
        }
        nk_input_end(context);

        if (nk_begin(
            context,
            "Exchatge",
            nk_rect(0, 0, (float) width, (float) height),
            NK_WINDOW_BORDER
        )) {
            nk_layout_row_dynamic(context, (float) height / 2, 1);
            nk_label(context, message ? message : "Unable to connect", NK_TEXT_CENTERED);
        }
        nk_end(context);

        SDL_SetRenderDrawColor(
            renderer,
            (int) colorf.r * 255,
            (int) colorf.g * 255,
            (int) colorf.b * 255,
            (int) colorf.a * 255
        );
        SDL_RenderClear(renderer);

        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }

    // clean up

    cleanup:

    SDL_free(message);

    nk_sdl_shutdown();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDLNet_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
