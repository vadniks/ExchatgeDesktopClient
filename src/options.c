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

static Options* options = NULL;

struct Options_t {
    bool admin;
    char* host;
    unsigned port;
    byte* serverSignPublicKey;
};

static char** readOptionsFile(unsigned* linesCountBuffer) {
    SDL_RWops* rwOps = SDL_RWFromFile(OPTIONS_FILE, "r");
    assert(rwOps);

    char buffer[MAX_FILE_SIZE];
    const unsigned actualFileSize = SDL_RWread(rwOps, buffer, 1, MAX_FILE_SIZE);
    assert(actualFileSize);
    SDL_RWclose(rwOps);

    char** lines = NULL;
    unsigned linesCount = 0;

    char* line = NULL;
    for (unsigned i = 0, j = 0; i <= actualFileSize; i++) {
        if (buffer[i] == '\n' || i == actualFileSize) {
            line = SDL_realloc(line, ++j);
            line[j] = 0;

            lines = SDL_realloc(lines, ++linesCount * sizeof(void*));
            lines[linesCount - 1] = line;

            j = 0;
            line = NULL;
        } else {
            line = SDL_realloc(line, ++j);
            line[j - 1] = buffer[i];
        }
    }

    *linesCountBuffer = linesCount;
    return lines;
}

#include "stdio.h"

static void parseAdminOption(const char* line) {
    options->admin = SDL_strstr(line, "true");
    printf("admin %s %lu\n", line, SDL_strlen(line));
}

static void parseHostOption(const char* line) {
    printf("host %s %lu\n", line, SDL_strlen(line));
}

static void parsePortOption(const char* line) {
    printf("port %s %lu\n", line, SDL_strlen(line));
}

static void parseSskpOption(const char* line) {
    printf("sskp %s %lu\n", line, SDL_strlen(line));
}

void optionsInit(void) {
    assert(!options);
    options = SDL_malloc(sizeof *options);

    unsigned linesCount = 0;
    char** lines = readOptionsFile(&linesCount);

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

void optionsClear(void) {
    assert(options);
//    SDL_free(options->host);
//    SDL_free(options->serverSignPublicKey);
    SDL_free(options);
}
