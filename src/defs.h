
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

extern const char* APP_TITLE;
extern const char* NET_HOST;

extern const unsigned APP_WIDTH;
extern const unsigned APP_HEIGHT;
extern const unsigned UI_UPDATE_PERIOD;
extern const unsigned NET_UPDATE_PERIOD;
