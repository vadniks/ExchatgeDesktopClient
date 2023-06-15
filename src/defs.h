
#pragma once

#define THIS(x) \
    typedef struct { x } This; \
    static This* this = NULL;

#ifdef __clang__
#   define nullable _Nullable
#   define staticAssert(x) _Static_assert(x, "")
#else
#   define nullable
#   define staticAssert(x)
#endif

typedef void* (*Function)(void*);
typedef unsigned char byte;
