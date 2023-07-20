
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
