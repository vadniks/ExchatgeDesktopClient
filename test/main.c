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
#include "testCollections.h"

int main(int argc, const char* const* argv) {
    assert(argc == 2);
    assert(!SDL_Init(0));

    switch (SDL_atoi(argv[1])) {
        case 0: testCollections_listBasic(); break;
        case 1: testCollections_listExtra(); break;
        case 2: testCollections_queueBasic(); break;
        case 3: testCollections_queueExtra(); break;
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}
