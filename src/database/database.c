
//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <sdl/SDL_stdinc.h>
#include <sdl/SDL_rwops.h>
#include <assert.h>
#include <unistd.h>
#include "crypto.h"
#include "database.h"

STATIC_CONST_STRING FILE_NAME = "database.sqlite3";

THIS(
    sqlite3* db;
    Crypto* crypto;
)

static void initFromExisted(byte* passwordBuffer, unsigned size) {
    byte streamsStates[CRYPTO_STREAMS_STATES_SIZE];
//    SDL_LoadFile() // TODO

    byte* key = cryptoMakeKey(passwordBuffer, size);
    SDL_free(key);
}

static void init(byte* passwordBuffer, unsigned size) {

}

bool databaseInit(byte* passwordBuffer, unsigned size) {
    this = SDL_malloc(sizeof *this);

    bool existedEarlier = !access(FILE_NAME, F_OK | R_OK | W_OK);
    if (sqlite3_open(FILE_NAME, &(this->db)) != 0) return false;

    if (existedEarlier) initFromExisted(passwordBuffer, size);
    else init(passwordBuffer, size);

    cryptoFillWithRandomBytes(passwordBuffer, size);
    SDL_free(passwordBuffer);

    return true;
}

void databaseClean(void) {
    assert(!sqlite3_close(this->db));
    SDL_free(this);
}
