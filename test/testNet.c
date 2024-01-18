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
#include "../src/net.h"
#include "testNet.h"

void testNet_packMessage(void) {
    const int allocations = SDL_GetNumAllocations();

    const int size = 2;
    byte body[size];
    SDL_memset(body, 8, sizeof body);

    ExposedTestNet_Message msg = {
        0,
        1,
        size,
        3,
        4,
        5,
        6,
        {0},
        body
    };
    SDL_memset(msg.token, 7, sizeof msg.token);

    byte* packed = exposedTestNet_packMessage(&msg);
    assert(packed);

    unsigned long test = 0x0123456789abcdef;
    assert(sizeof(byte) == 1 && sizeof(int) == 4 && sizeof(long) == 8 && *((byte*) &test) == 0xef);

    assert(*((int*) packed) == 0);
    assert(*(long*) (packed + 4) == 1);
    assert(*(int*) (packed + 4 + 8) == 2);
    assert(*(int*) (packed + 4 * 2 + 8) == 3);
    assert(*(int*) (packed + 4 * 3 + 8) == 4);
    assert(*(int*) (packed + 4 * 4 + 8) == 5);
    assert(*(int*) (packed + 4 * 5 + 8) == 6);
    assert(!SDL_memcmp(packed + sizeof(int) * 6 + sizeof(long), msg.token, sizeof msg.token));
    assert(!SDL_memcmp(packed + sizeof(int) * 6 + sizeof(long) + sizeof msg.token, body, sizeof body));

    SDL_free(packed);

    assert(allocations == SDL_GetNumAllocations());
}

void testNet_unpackMessage(void) {
    const int allocations = SDL_GetNumAllocations();

    const byte akaPacked[98] = { // Copy bytes from memory view of the array from the above defined function in the GDB
        0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0,
        0x3, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x6, 0x0, 0x0, 0x0,
        0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
        0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
        0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
        0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
        0x8, 0x8
    };

    ExposedTestNet_Message* msg = exposedTestNet_unpackMessage(akaPacked);
    assert(msg);

    assert(msg->flag == 0);
    assert(msg->timestamp == 1);
    assert(msg->size == 2);
    assert(msg->index == 3);
    assert(msg->count == 4);
    assert(msg->from == 5);
    assert(msg->to == 6);

    byte buffer[msg->size];

    SDL_memset(buffer, 7, sizeof msg->token);
    assert(!SDL_memcmp(msg->token, buffer, sizeof msg->token));

    SDL_memset(buffer, 8, msg->size);
    assert(!SDL_memcmp(msg->body, buffer, msg->size));

    SDL_free(msg->body);
    SDL_free(msg);

    assert(allocations == SDL_GetNumAllocations());
}
