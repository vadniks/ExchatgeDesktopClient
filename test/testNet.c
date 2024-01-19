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

#pragma ide diagnostic ignored "misc-no-recursion"

#include <assert.h>
#include <SDL.h>
#include <SDL_net.h>
#include <time.h>
#include <unistd.h>
#include "../src/net.h"
#include "testNet.h"

static int akaServerThread(void*) {
    IPaddress address = {INADDR_NONE, SDL_Swap16(8080)};
    TCPsocket server = SDLNet_TCP_Open(&address);
    assert(server);

    bool clientConnected = false;
    const time_t started = time(NULL);

    while (difftime(time(NULL), started) <= 5.0) {
        TCPsocket client = SDLNet_TCP_Accept(server);
        if (!client) continue;
        clientConnected = true;

        const int size = 5;
        byte buffer[size];

        SDL_memset(buffer, 0, size);
        assert(SDLNet_TCP_Recv(client, buffer, size) == size);
        assert(!SDL_memcmp(buffer, (byte[size]) {1, 1, 1, 1, 1}, size));

        SDL_memset(buffer, 2, size);
        assert(SDLNet_TCP_Send(client, buffer, size) == size);

        SDLNet_TCP_Close(client);
        break;
    }
    SDLNet_TCP_Close(server);

    assert(clientConnected);
    return 0;
}

void testNet_basic(void) {
    staticAssert(BYTE_ORDER == LITTLE_ENDIAN);

    const int allocations = SDL_GetNumAllocations();
    SDLNet_Init();

    SDL_Thread* akaServer = SDL_CreateThread(&akaServerThread, "0", NULL);
    sleep(1);

    {
        IPaddress address = {SDL_Swap32(INADDR_LOOPBACK), SDL_Swap16(8080)};
        TCPsocket socket = SDLNet_TCP_Open(&address);
        assert(socket);

        const int size = 5;
        byte buffer[size];

        SDL_memset(buffer, 1, size);
        assert(SDLNet_TCP_Send(socket, buffer, size) == size);

        SDL_memset(buffer, 0, size);
        assert(SDLNet_TCP_Recv(socket, buffer, size) == size);
        assert(!SDL_memcmp(buffer, (byte[size]) {2, 2, 2, 2, 2}, size));

        SDLNet_TCP_Close(socket);
    }

    SDL_WaitThread(akaServer, NULL);

    SDLNet_Quit();
    assert(allocations == SDL_GetNumAllocations());
}

void testNet_packMessage(bool first) {
    const int allocations = SDL_GetNumAllocations();

    const int size = 2;
    byte body[size];
    SDL_memset(body, 8, sizeof body);

    ExposedTestNet_Message msg = {0, 1, first ? size : 0, 3, 4, 5, 6, {0}, first ? body : NULL};
    SDL_memset(msg.token, 7, sizeof msg.token);

    byte* packed = exposedTestNet_packMessage(&msg);
    assert(packed);

    unsigned long test = 0x0123456789abcdef;
    assert(sizeof(byte) == 1 && sizeof(int) == 4 && sizeof(long) == 8 && *((byte*) &test) == 0xef);

    assert(*((int*) packed) == 0);
    assert(*(long*) (packed + 4) == 1);
    assert(*(int*) (packed + 4 + 8) == (first ? 2 : 0));
    assert(*(int*) (packed + 4 * 2 + 8) == 3);
    assert(*(int*) (packed + 4 * 3 + 8) == 4);
    assert(*(int*) (packed + 4 * 4 + 8) == 5);
    assert(*(int*) (packed + 4 * 5 + 8) == 6);
    assert(!SDL_memcmp(packed + sizeof(int) * 6 + sizeof(long), msg.token, sizeof msg.token));
    first ? assert(!SDL_memcmp(packed + sizeof(int) * 6 + sizeof(long) + sizeof msg.token, body, sizeof body)) : STUB;

    SDL_free(packed);

    assert(allocations == SDL_GetNumAllocations());
    first ? testNet_packMessage(false) : STUB;
}

void testNet_unpackMessage(bool first) {
    const int allocations = SDL_GetNumAllocations();

    const byte akaPacked[98] = { // Copy bytes from memory view of the array from the above defined function in the GDB
        0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, first ? 0x2 : 0x0, 0x0, 0x0, 0x0,
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
    assert(msg->size == (first ? 2 : 0));
    assert(msg->index == 3);
    assert(msg->count == 4);
    assert(msg->from == 5);
    assert(msg->to == 6);

    byte buffer[msg->size];

    SDL_memset(buffer, 7, sizeof msg->token);
    assert(!SDL_memcmp(msg->token, buffer, sizeof msg->token));

    SDL_memset(buffer, 8, msg->size);
    first ? assert(!SDL_memcmp(msg->body, buffer, msg->size)) : assert(!msg->body);

    SDL_free(msg->body);
    SDL_free(msg);

    assert(allocations == SDL_GetNumAllocations());
    first ? testNet_unpackMessage(false) : STUB;
}

void testNet_unpackUserInfo(void) {
    const byte akaPacked[21] = {
        1, 0, 0,0, 1, 't', 'e', 's', 't', 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0
    };

    ExposedTestNet_UserInfo* info = exposedTestNet_unpackUserInfo(akaPacked);
    assert(info->id == akaPacked[0]);
    assert(info->connected == akaPacked[4]);
    assert(!SDL_strcmp((char*) info->name, (char*) (akaPacked + 4 + 1)));
    SDL_free(info);
}
