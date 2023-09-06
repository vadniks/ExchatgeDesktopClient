/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
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

#include <SDL.h>
#include <assert.h>
#include "options.h"

STATIC_CONST_STRING OPTIONS_FILE = "options.txt";
STATIC_CONST_UNSIGNED MAX_FILE_SIZE = 1 << 10;

STATIC_CONST_STRING ADMIN_OPTION = "admin";
STATIC_CONST_STRING HOST_OPTION = "host";
STATIC_CONST_STRING PORT_OPTION = "port";
STATIC_CONST_STRING SSPK_OPTION = "sspk";

typedef struct {
    bool admin;
    char* host;
    unsigned port;
    byte* serverSignPublicKey;
    unsigned serverSignPublicKeySize;
} Options;

static Options* options = NULL;

static char** nullable readOptionsFile(unsigned* linesCountBuffer) {
    SDL_RWops* rwOps = SDL_RWFromFile(OPTIONS_FILE, "r");
    if (!rwOps) return NULL;

    char buffer[MAX_FILE_SIZE];
    const unsigned actualFileSize = SDL_RWread(rwOps, buffer, 1, MAX_FILE_SIZE);
    SDL_RWclose(rwOps);
    if (!actualFileSize) return NULL;

    char** lines = NULL;
    unsigned linesCount = 0;

    char* line = NULL;
    for (unsigned i = 0, j = 0; i <= actualFileSize; i++) {
        line = SDL_realloc(line, ++j);

        if (buffer[i] == '\n' || i == actualFileSize) {
            line[j - 1] = 0;

            lines = SDL_realloc(lines, ++linesCount * sizeof(void*));
            lines[linesCount - 1] = line;

            j = 0;
            line = NULL;
        } else
            line[j - 1] = buffer[i];
    }

    *linesCountBuffer = linesCount;
    return lines;
}

static void parseAdminOption(const char* line)
{ options->admin = SDL_strstr(line, "true"); }

static void parseHostOption(const char* line) {
    const unsigned size = SDL_strlen(line),
        declarationSize = SDL_strlen(HOST_OPTION) + 1,
        payloadSize = size - declarationSize;

    options->host = SDL_malloc(payloadSize);
    SDL_memcpy(options->host, line + declarationSize, payloadSize);
}

static void parsePortOption(const char* line)
{ options->port = SDL_atoi(line + SDL_strlen(HOST_OPTION) + 1); }

static void parseSskpOption(const char* line) {
    const unsigned size = SDL_strlen(line);

    byte* nums = NULL;
    unsigned count = 0;
    char* num = NULL;

    for (unsigned i = SDL_strlen(SSPK_OPTION) + 1, j = 0; i <= size; i++) {
        num = SDL_realloc(num, ++j);

        if (line[i] == ',' || i == size) {
            num[j - 1] = 0;

            nums = SDL_realloc(nums, ++count);
            nums[count - 1] = (byte) SDL_atoi(num);

            SDL_free(num);
            num = NULL;
            j = 0;
        } else
            num[j - 1] = line[i];
    }

    options->serverSignPublicKey = nums;
    options->serverSignPublicKeySize = count;
}

bool optionsInit(void) {
    assert(!options);
    options = SDL_malloc(sizeof *options);
    options->admin = false;
    options->host = NULL;
    options->port = 0;
    options->serverSignPublicKey = NULL;

    unsigned linesCount = 0;
    char** lines = readOptionsFile(&linesCount);
    if (!lines) {
        SDL_free(options);
        options = NULL;
        return false;
    }

    char* line;
    for (unsigned i = 0; i < linesCount; i++) {
        line = lines[i];

        if (SDL_strstr((char*) line, ADMIN_OPTION)) parseAdminOption(line);
        else if (SDL_strstr((char*) line, HOST_OPTION)) parseHostOption(line);
        else if (SDL_strstr((char*) line, PORT_OPTION)) parsePortOption(line);
        else if (SDL_strstr((char*) line, SSPK_OPTION)) parseSskpOption(line);
        else assert(false);

        SDL_free(line);
    }
    SDL_free(lines);

    return true;
}

bool optionsIsAdmin(void) {
    assert(options);
    return options->admin;
}

const char* optionsHost(void) {
    assert(options);
    return options->host;
}

unsigned optionsPort(void) {
    assert(options);
    return options->port;
}

const byte* optionsServerSignPublicKey(void) {
    assert(options);
    return options->serverSignPublicKey;
}

unsigned optionsServerSignPublicKeySize(void) {
    assert(options);
    return options->serverSignPublicKeySize;
}

void optionsClear(void) {
    assert(options);
    SDL_free(options->host);
    SDL_free(options->serverSignPublicKey);
    SDL_free(options);
}
