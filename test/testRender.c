/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <SDL.h>
#include "testRender.h"

void testRender_sdlRendererBasic(void) {
    SDL_Window* window = SDL_CreateWindow(
        "Title",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        160,
        90,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    assert(window);

    SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    assert(renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
