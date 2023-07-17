
//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <sdl/SDL_stdinc.h>
#include <sdl/SDL_log.h> // TODO: test only
#include <sdl/SDL_mutex.h>
#include <assert.h>
#include <unistd.h>
#include "database.h"

#define SYNCHRONIZED_BEGIN SDL_LockMutex(this->mutex);
#define SYNCHRONIZED_END SDL_UnlockMutex(this->mutex);
#define SYNCHRONIZED(x) SYNCHRONIZED_BEGIN x SYNCHRONIZED_END

STATIC_CONST_STRING FILE_NAME = "./database.sqlite3";

STATIC_CONST_STRING SERVICE_TABLE = "service"; // 7
STATIC_CONST_STRING STREAMS_STATES_COLUMN = "streamsStates"; // 13

THIS(
    SDL_mutex* mutex;
    sqlite3* db;
    Crypto* crypto;
)

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

static void insertEncryptedStreamStatesBinder(const byte* encryptedStreamsStates, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_blob(statement, 1, encryptedStreamsStates, (int) cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), SQLITE_STATIC)); }

static void insertEncryptedStreamsStates(const byte* encryptedStreamsStates) {
    assert(!streamsStatesExists());

    const unsigned sqlSize = 47;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "insert into %s (%s) values (?)", SERVICE_TABLE, STREAMS_STATES_COLUMN) == sqlSize - 1);

    execSingle(sql, sqlSize, (StatementProcessor) &insertEncryptedStreamStatesBinder, (void*) encryptedStreamsStates, NULL, NULL);
}

static Crypto* nullable init(byte* passwordBuffer, unsigned size, const byte* nullable encryptedStreamsStates) {
    createTableIfNotExists();

    Crypto* crypto = cryptoInit();
    byte* key = cryptoMakeKey(passwordBuffer, size);

    if (encryptedStreamsStates) {
        byte* streamsStates = cryptoDecryptSingle(key, encryptedStreamsStates, cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE));
        if (!streamsStates) {
            SDL_free(key);
            cryptoDestroy(crypto);
            return NULL;
        }

        cryptoSetUpAutonomous(crypto, key, streamsStates);
        SDL_free(streamsStates);
    } else
        cryptoSetUpAutonomous(crypto, key, NULL);

    cryptoFillWithRandomBytes(key, CRYPTO_KEY_SIZE);
    SDL_free(key);

    return crypto;
}

static Crypto* nullable initFromExisted(byte* passwordBuffer, unsigned size) {
    const unsigned sqlSize = 34;
    char sql[sqlSize + 1];
    assert(SDL_snprintf(sql, sqlSize, "select %s from %s", STREAMS_STATES_COLUMN, SERVICE_TABLE) == sqlSize - 1);

    sqlite3_stmt* statement;
    assert(!sqlite3_prepare(this->db, sql, (int) sqlSize, &statement, NULL));
    assert(sqlite3_step(statement) == SQLITE_ROW);

    const unsigned encryptedStreamsStatesSize = cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE);
    const byte* encryptedStreamsStates = sqlite3_column_blob(statement, 0);
    assert(encryptedStreamsStates && sqlite3_column_bytes(statement, 0) == (int) encryptedStreamsStatesSize);

    Crypto* crypto = init(passwordBuffer, size, encryptedStreamsStates);
    assert(!sqlite3_finalize(statement));

    return crypto;
}

static Crypto* initNew(byte* passwordBuffer, unsigned size) {
    Crypto* crypto = init(passwordBuffer, size, NULL);
    assert(crypto);

    byte* streamsStates = cryptoExportStreamsStates(crypto);
    byte* encryptedStreamsStates = cryptoEncryptSingle(cryptoClientKey(crypto), streamsStates, CRYPTO_STREAMS_STATES_SIZE);
    SDL_free(streamsStates);

    insertEncryptedStreamsStates(encryptedStreamsStates);
    SDL_free(encryptedStreamsStates);

    return crypto;
}

bool databaseInit(byte* passwordBuffer, unsigned size) {
    assert(!this && size > 0);
    this = SDL_malloc(sizeof *this);
    this->mutex = SDL_CreateMutex();
    SYNCHRONIZED_BEGIN

    bool existedEarlier = !access(FILE_NAME, F_OK | R_OK | W_OK);
    if (sqlite3_open(FILE_NAME, &(this->db)) != 0) {
        SYNCHRONIZED_END
        cryptoFillWithRandomBytes(passwordBuffer, size);
        SDL_free(passwordBuffer);
        return false;
    }

    if (existedEarlier) this->crypto = initFromExisted(passwordBuffer, size);
    else this->crypto = initNew(passwordBuffer, size);
    assert(streamsStatesExists());

    SDL_free(passwordBuffer);
    SYNCHRONIZED_END

    if (!this->crypto) return false;
    else return true;
}

void databaseClean(void) {
    assert(this && !sqlite3_close(this->db));
    if (this->crypto) cryptoDestroy(this->crypto);
    SDL_DestroyMutex(this->mutex);
    SDL_free(this);
}
