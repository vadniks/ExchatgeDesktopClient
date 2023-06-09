
#pragma once

#define THIS(x) \
    typedef struct { x } This; \
    static This* this = NULL;

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define boolToStr(x) (x ? "true" : "false")
#define printArray(x, y) puts(#x); for (unsigned i = 0; i < (y); printf("%d ", (x)[i++])); puts("");

#define staticAssert(x) _Static_assert(x, "")

#if !defined(__GNUC__) // && !defined(__clang__) as clang defines __GNUC__ too
#   error "Don't go any further"
#endif

#ifndef __LINUX__
#   warning "Project targets linux systems"
#endif

#ifndef __x86_64__
#   warning "Project targets x86_64 systems"
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
