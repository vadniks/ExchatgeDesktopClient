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
#include "../src/defs.h"
#include "testCollections.h"
#include "testRender.h"
#include "testNet.h"
#include "testCrypto.h"

#pragma pack(true)

#ifndef TESTING
#   error "Enable in buildscript"
#endif

int main(int argc, const char* const* argv) {
    staticAssert(__LINUX__ == 1);
    assert(argc == 2);
    assert(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER));
    testCrypto_start();

    switch (SDL_atoi(argv[1])) {
        case 0: testCollections_listBasic(); break;
        case 1: testCollections_listExtra(); break;
        case 2: testCollections_queueBasic(); break;
        case 3: testCollections_queueExtra(); break;

        case 4: testRender_sdlRendererBasic(); break;

        case 5: testNet_basic(); break;
        case 6: testNet_packMessage(true); break;
        case 7: testNet_unpackMessage(true); break;
        case 8: testNet_unpackUserInfo(); break;

        case 9: testCrypto_keyExchange(); break;
        case 10: testCrypto_signature(); break;
        case 11: testCrypto_streamCrypt(); break;
        case 12: testCrypto_singleCrypt(); break;
        case 13: testCrypto_hash(); break;
    }

    testCrypto_stop();
    SDL_Quit();
    return EXIT_SUCCESS;
}
