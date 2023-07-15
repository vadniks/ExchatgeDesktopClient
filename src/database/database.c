
//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <sdl/SDL_stdinc.h>
#include <sdl/SDL_log.h> // TODO: test only
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

struct DatabaseMessage_t {
    unsigned* nullable userId;
    unsigned long timestamp;
    char* text;
    unsigned size;
};

typedef void (*StatementProcessor)(void* nullable, sqlite3_stmt*);

static void execSingle(
    const char* sql,
    unsigned sqlSize,
    StatementProcessor nullable binder,
    void* nullable binderParameter,
    StatementProcessor nullable resultHandler,
    void* nullable resultHandlerParameter
) {
    sqlite3_stmt* statement;
    assert(!sqlite3_prepare(this->db, sql, (int) sqlSize, &statement, NULL));
    if (binder) (*binder)(binderParameter, statement);

    if (!resultHandler) assert(sqlite3_step(statement) == SQLITE_DONE);
    else (*resultHandler)(resultHandlerParameter, statement);

    assert(!sqlite3_finalize(statement));
}

static void createTableIfNotExists(void) {
    const unsigned sqlSize = 65;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "create table if not exists %s (%s blob not null)", SERVICE_TABLE, STREAMS_STATES_COLUMN) == sqlSize - 1);

    execSingle(sql, sqlSize, NULL, NULL, NULL, NULL);
}

static void streamsStatesExistsResultHandler(bool* result, sqlite3_stmt* statement) {
    assert(sqlite3_step(statement) == SQLITE_ROW);
    *result = sqlite3_column_int(statement, 0) > 0;
}

static bool streamsStatesExists(void) {
    const unsigned sqlSize = 29;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "select count(*) from %s", SERVICE_TABLE) == sqlSize - 1);

    bool result = false;
    execSingle(sql, sqlSize, NULL, NULL, (StatementProcessor) &streamsStatesExistsResultHandler, &result);
    return result;
}

static void insertStreamStatesBinder(const byte* streamsStates, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_blob(statement, 1, streamsStates, (int) CRYPTO_STREAMS_STATES_SIZE, SQLITE_STATIC)); }

static void insertStreamsStates(const byte* streamsStates) {
    assert(!streamsStatesExists());

    const unsigned sqlSize = 47;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "insert into %s (%s) values (?)", SERVICE_TABLE, STREAMS_STATES_COLUMN) == sqlSize - 1);

    execSingle(sql, sqlSize, (StatementProcessor) &insertStreamStatesBinder, (void*) streamsStates, NULL, NULL);
}

static Crypto* init(byte* passwordBuffer, unsigned size, const byte* nullable streamsStates) {
    createTableIfNotExists();

    Crypto* crypto = cryptoInit();
    byte* key = cryptoMakeKey(passwordBuffer, size);
    cryptoSetUpAutonomous(crypto, key, streamsStates);
    cryptoFillWithRandomBytes(key, CRYPTO_KEY_SIZE);
    SDL_free(key);

    return crypto;
}

static Crypto* initFromExisted(byte* passwordBuffer, unsigned size) {
    const unsigned sqlSize = 34;
    char sql[sqlSize + 1];
    assert(SDL_snprintf(sql, sqlSize, "select %s from %s", STREAMS_STATES_COLUMN, SERVICE_TABLE) == sqlSize - 1);

    sqlite3_stmt* statement;
    assert(!sqlite3_prepare(this->db, sql, (int) sqlSize, &statement, NULL));
    assert(sqlite3_step(statement) == SQLITE_ROW);

    const byte* streamsStates = sqlite3_column_blob(statement, 0);
    assert(streamsStates && sqlite3_column_bytes(statement, 0) == (int) CRYPTO_STREAMS_STATES_SIZE);
    assert(!sqlite3_finalize(statement));

    return init(passwordBuffer, size, streamsStates);
}

static Crypto* initNew(byte* passwordBuffer, unsigned size) {
    Crypto* crypto = init(passwordBuffer, size, NULL);

    byte* streamsStates = cryptoExportStreamsStates(crypto);
    insertStreamsStates(streamsStates);
    SDL_free(streamsStates);

    return crypto;
}

bool databaseInit(byte* passwordBuffer, unsigned size) {
    assert(!this);
    this = SDL_malloc(sizeof *this);

    bool existedEarlier = !access(FILE_NAME, F_OK | R_OK | W_OK);
    if (sqlite3_open(FILE_NAME, &(this->db)) != 0) return false;

    if (existedEarlier) this->crypto = initFromExisted(passwordBuffer, size);
    else this->crypto = initNew(passwordBuffer, size);
    assert(streamsStatesExists());

    cryptoFillWithRandomBytes(passwordBuffer, size);
    SDL_free(passwordBuffer);

    return true;
}

void databaseClean(void) {
    assert(this && !sqlite3_close(this->db));
    cryptoDestroy(this->crypto);
    SDL_free(this);
}
