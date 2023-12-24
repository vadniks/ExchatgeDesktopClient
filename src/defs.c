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

#include <sys/resource.h>
#include <stdio.h> // not SDL_log to avoid depending on SDL as it needs to be (de)initialized
#include <execinfo.h>
#include <stdlib.h>
#include "defs.h"

void xPrintBinaryArray(const char* name, const void* array, unsigned size) {
    printf("%s: ", name);
    for (unsigned i = 0; i < size; printf("%x ", ((const byte*) array)[i++]));
    printf("\n");
}

unsigned stackLimit(void) {
    struct rlimit rlimit;
    getrlimit(RLIMIT_STACK, &rlimit);
    return (unsigned) rlimit.rlim_cur;
}

void printStackTrace(void) {
    const int size = 0xf;
    void* array[size];

    int actualSize = backtrace(array, size);
    char** strings = backtrace_symbols(array, actualSize);
    if (!strings) return;

    for (int i = 0; i < actualSize; i ? puts(strings[i++]) : i++);
    puts("");
    free(strings);
}
