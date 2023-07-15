
//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include <unistd.h>
#include "crypto.h"
#include "database.h"

STATIC_CONST_STRING FILE_NAME = "./database.sqlite3";

STATIC_CONST_STRING SERVICE_TABLE = "service"; // 7
STATIC_CONST_STRING STREAMS_STATES_COLUMN = "streamsStates"; // 13

THIS(
    sqlite3* db;
    Crypto* crypto;
)

static Crypto* init(byte* passwordBuffer, unsigned size, const byte* nullable streamsStates) {
    Crypto* crypto = cryptoInit();
    byte* key = cryptoMakeKey(passwordBuffer, size);
    cryptoSetUpAutonomous(crypto, key, streamsStates);
    cryptoFillWithRandomBytes(key, CRYPTO_KEY_SIZE);
    SDL_free(key);

    const unsigned sqlSize = 64;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "create table if not exists %s (%s blob not null)", SERVICE_TABLE, STREAMS_STATES_COLUMN) == sqlSize);

    sqlite3_stmt* statement;
    assert(!sqlite3_prepare(this->db, sql, (int) sqlSize, &statement, NULL));
    assert(sqlite3_step(statement) == SQLITE_DONE);
    assert(!sqlite3_finalize(statement));

    return crypto;
}

static Crypto* initFromExisted(byte* passwordBuffer, unsigned size) {
    const unsigned sqlSize = 34;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "select %s from %s", STREAMS_STATES_COLUMN, SERVICE_TABLE) == sqlSize);

    sqlite3_stmt* statement;
    assert(!sqlite3_prepare(this->db, sql, (int) sqlSize, &statement, NULL));
    assert(sqlite3_step(statement) == SQLITE_ROW);

    const byte* streamsStates = sqlite3_column_blob(statement, 0);
    assert(streamsStates && sqlite3_column_bytes(statement, 0) == (int) CRYPTO_STREAMS_STATES_SIZE);
    assert(!sqlite3_finalize(statement));

    return init(passwordBuffer, size, streamsStates);
}

bool databaseInit(byte* passwordBuffer, unsigned size) {
    this = SDL_malloc(sizeof *this);

    bool existedEarlier = !access(FILE_NAME, F_OK | R_OK | W_OK);
    if (sqlite3_open(FILE_NAME, &(this->db)) != 0) return false;

    if (existedEarlier) initFromExisted(passwordBuffer, size);
    else init(passwordBuffer, size, NULL);

    cryptoFillWithRandomBytes(passwordBuffer, size);
    SDL_free(passwordBuffer);

    return true;
}

void databaseClean(void) {
    assert(!sqlite3_close(this->db));
    SDL_free(this);
}
