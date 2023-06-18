
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

#define STATIC_CONST_INT static const int
#define STATIC_CONST_UNSIGNED static const unsigned

typedef void* (*Function)(void*);
typedef unsigned char byte;
