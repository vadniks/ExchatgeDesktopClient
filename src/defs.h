
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

extern const char* APP_TITLE;
extern const char* NET_HOST;

#define NET_MESSAGE_BODY_SIZE (1 << 10) // 1024

extern const int APP_WIDTH;
extern const int APP_HEIGHT;
extern const int UI_UPDATE_PERIOD;
extern const int NET_UPDATE_PERIOD;
extern const int NET_PORT;
extern const int NET_MESSAGE_HEAD_SIZE;
extern const int NET_RECEIVE_BUFFER_SIZE;
extern const int NET_FLAG_FINISH;

typedef struct {
    // begin head
    int flag; // short service description of Message
    unsigned long timestamp; // Message created at
    unsigned size; // actual size of th payload
    unsigned index; // Message part index if the whole Message cannot fit in boy
    unsigned count; // total count of Message parts
    // end head
    byte body[NET_MESSAGE_BODY_SIZE]; // payload
} Message;
