
#pragma once

#define THIS(x) \
    typedef struct { x } this_t; \
    static this_t* this = NULL;

#ifdef __clang__
#   define nullable _Nullable
#   define staticAssert(x) _Static_assert(x, "")
#else
#   define nullable
#   define staticAssert(x)
#endif

typedef void* (*function)(void*);
typedef unsigned char byte;

__attribute__((deprecated)) typedef struct {
    const byte* chars;
    unsigned length;
} string;

extern const char* APP_TITLE;
extern const char* NET_HOST;

extern const int APP_WIDTH;
extern const int APP_HEIGHT;
extern const int UI_UPDATE_PERIOD;
extern const int NET_UPDATE_PERIOD;
extern const int NET_PORT;
extern const int NET_RECEIVE_BUFFER_SIZE;
extern const int NET_MESSAGE_HEAD_SIZE;
extern const int NET_MESSAGE_BODY_SIZE;
