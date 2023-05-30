
#pragma once

#define THIS(x) \
    typedef struct { x } this_t; \
    static this_t* this = NULL;

typedef void* (*function)(void*);
typedef unsigned char byte;

typedef struct {
    const byte* chars;
    unsigned length;
} string;

extern const char* APP_TITLE;

extern const int APP_WIDTH;
extern const int APP_HEIGHT;
extern const int UI_UPDATE_PERIOD;
