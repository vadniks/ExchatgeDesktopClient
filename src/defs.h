
#pragma once

#define THIS(x) \
    typedef struct { x } This; \
    static This* this = NULL;

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)

#define staticAssert(x) _Static_assert(x, "")

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
