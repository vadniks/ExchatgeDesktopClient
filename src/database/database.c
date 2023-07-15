
//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "../defs.h"
#include "database.h"

STATIC_CONST_STRING FILE_NAME = "database.sqlite3";

THIS(
    sqlite3* db;
)

bool databaseInit(void) {
    this = SDL_malloc(sizeof *this);

    return !sqlite3_open(FILE_NAME, &(this->db));
}

void databaseClean(void) {
    assert(!sqlite3_close(this->db));
    SDL_free(this);
}
