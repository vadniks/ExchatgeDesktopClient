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

#if !defined(__STDC__)
#   error "How's that?"
#endif

#if __STDC_VERSION__ < 201112L // C11
#   error "New C features are used"
#endif

#if !defined(__GNUC__) // && !defined(__clang__) as clang defines __GNUC__ too
#   error "Project uses gnu extensions"
#endif

#ifdef __STDC_NO_ATOMICS__
#   error "Gimme atomics!"
#endif

#define THIS(x) \
    typedef struct { x } This; \
    static This* this = NULL;

#define staticAssert(x) _Static_assert(x, "")
#define atomic _Atomic
#define fallthrough __attribute__((fallthrough));
#define type_of(x) __typeof__(x) // IDE shows error if plain typeof is used, so fallback should be used instead
#define auto_type __auto_type

#pragma clang deprecated(type_of, "")
#pragma clang deprecated(auto_type, "")

#ifndef __GLIBC__
#   error "Project uses glibc"
#endif

#ifndef __linux__
#   error "Project targets linux systems"
#endif

#if __SIZEOF_POINTER__ != 8
#   error "Project targets 64 bits sized pointers"
#endif

#ifdef __clang__
#   define nullable _Nullable // all pointers without nullable attribute in their declarations are treated as non-nullable pointers in this project
#else
#   define nullable
#endif

typedef unsigned char byte;

void xPrintBinaryArray(const char* name, const void* array, unsigned size);
unsigned stackLimit(void);
void printStackTrace(void);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#   define min(x, y) (x < y ? x : y)
#   define max(x, y) (x > y ? x : y)
#   define boolToStr(x) (x ? "true" : "false")
#   define printBinaryArray(x, y) xPrintBinaryArray(#x, x, y)
#   define xAlloca(x) ((void* nullable) (x <= (unsigned) ((float) stackLimit() * 0.85f) ? ((byte[x]) {}) : NULL))
#   define USED(x) ((void) x)
#   define STUB USED(0)
#pragma clang diagnostic pop

#define STATIC_CONST_UNSIGNED static const unsigned
#define STATIC_CONST_STRING static const char*
