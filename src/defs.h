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

#pragma once

#ifdef __cplusplus
#   error "Only the C - only hardcore!"
#endif

#define THIS(x) \
    typedef struct { x } This; \
    static This* this = NULL;

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define boolToStr(x) (x ? "true" : "false")
#define printBinaryArray(x, y) puts(#x); for (unsigned i = 0; i < (y); printf("%x ", ((const unsigned char*) x)[i++])); puts("");

#define staticAssert(x) _Static_assert(x, "")

#define atomic _Atomic
#define generic _Generic

#if !defined(__GNUC__) // && !defined(__clang__) as clang defines __GNUC__ too
#   error "Project uses gcc extensions"
#endif

#ifndef __GLIBC__
#   error "Project uses glibc"
#endif

#ifndef __LINUX__
#   error "Project targets linux systems"
#endif

#ifndef __x86_64__
#   error "Project targets x86_64 systems"
#endif

#ifdef __clang__
#   define nullable _Nullable // all pointers without nullable attribute in their declarations are treated as non-nullable pointers in this project
#else
#   define nullable
#endif

#define STATIC_CONST_INT static const int
#define STATIC_CONST_UNSIGNED static const unsigned
#define STATIC_CONST_STRING static const char*

typedef void* (*Function)(void*);
typedef unsigned char byte;
