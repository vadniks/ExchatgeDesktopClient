
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
STATIC_CONST_STRING CONVERSATIONS_TABLE = "conversations"; // 13
STATIC_CONST_STRING TIMESTAMP_COLUMN = "timestamp"; // 9
STATIC_CONST_STRING FROM_COLUMN = "\"from\""; // 6
STATIC_CONST_STRING TEXT_COLUMN = "text"; // 4
STATIC_CONST_STRING SIZE_COLUMN = "size"; // 4

THIS(
    SDL_mutex* mutex;
    unsigned passwordSize;
    unsigned usernameSize;
    sqlite3* db;
    Crypto* crypto;
)

typedef void (*StatementProcessor)(void* nullable, sqlite3_stmt*);

static void executeSingle(
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

static void createServiceTable(void) {
    const unsigned sqlSize = 51;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "create table %s (%s blob not null)", SERVICE_TABLE, STREAMS_STATES_COLUMN) == sqlSize - 1);

    executeSingle(sql, sqlSize, NULL, NULL, NULL, NULL);
}

static void createConversationsTable(void) {
    const unsigned bufferSize = 255;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table %s ("
            "%s unsigned bigint not null unique, " // timestamp
            "%s binary(%u) not null, " // from; value of %u may vary by display size, so large buffer is used
            "%s blob not null, " // text
            "%s unsigned int not null" // size
        ")",
        CONVERSATIONS_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN, this->usernameSize, TEXT_COLUMN, SIZE_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(sql, sqlSize, NULL, NULL, NULL, NULL);
}

static void createTables(void) {
    createServiceTable();
    createConversationsTable();
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
    executeSingle(sql, sqlSize, NULL, NULL, (StatementProcessor) &streamsStatesExistsResultHandler, &result);
    return result;
}

static void insertEncryptedStreamStatesBinder(const byte* encryptedStreamsStates, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_blob(statement, 1, encryptedStreamsStates, (int) cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), SQLITE_STATIC)); }

static void insertEncryptedStreamsStates(const byte* encryptedStreamsStates) {
    assert(!streamsStatesExists());

    const unsigned sqlSize = 47;
    char sql[sqlSize];
    assert(SDL_snprintf(sql, sqlSize, "insert into %s (%s) values (?)", SERVICE_TABLE, STREAMS_STATES_COLUMN) == sqlSize - 1);

    executeSingle(sql, sqlSize, (StatementProcessor) &insertEncryptedStreamStatesBinder, (void*) encryptedStreamsStates, NULL, NULL);
}

static Crypto* nullable init(byte* passwordBuffer, unsigned size, const byte* nullable encryptedStreamsStates) {
    if (!encryptedStreamsStates) createTables();

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

static Crypto* nullable initFromExisted(byte* passwordBuffer) {
    const unsigned sqlSize = 34;
    char sql[sqlSize + 1];
    assert(SDL_snprintf(sql, sqlSize, "select %s from %s", STREAMS_STATES_COLUMN, SERVICE_TABLE) == sqlSize - 1);

    sqlite3_stmt* statement;
    assert(!sqlite3_prepare(this->db, sql, (int) sqlSize, &statement, NULL));
    assert(sqlite3_step(statement) == SQLITE_ROW);

    const unsigned encryptedStreamsStatesSize = cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE);
    const byte* encryptedStreamsStates = sqlite3_column_blob(statement, 0);
    assert(encryptedStreamsStates && sqlite3_column_bytes(statement, 0) == (int) encryptedStreamsStatesSize);

    Crypto* crypto = init(passwordBuffer, this->passwordSize, encryptedStreamsStates);
    assert(!sqlite3_finalize(statement));

    return crypto;
}

static Crypto* initNew(byte* passwordBuffer) {
    Crypto* crypto = init(passwordBuffer, this->passwordSize, NULL);
    assert(crypto);

    byte* streamsStates = cryptoExportStreamsStates(crypto);
    byte* encryptedStreamsStates = cryptoEncryptSingle(cryptoClientKey(crypto), streamsStates, CRYPTO_STREAMS_STATES_SIZE);
    SDL_free(streamsStates);

    insertEncryptedStreamsStates(encryptedStreamsStates);
    SDL_free(encryptedStreamsStates);

    return crypto;
}

bool databaseInit(byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize) {
    assert(!this && passwordSize > 0);
    this = SDL_malloc(sizeof *this);
    this->mutex = SDL_CreateMutex();
    SYNCHRONIZED_BEGIN
    this->passwordSize = passwordSize;
    this->usernameSize = usernameSize;

    bool existedEarlier = !access(FILE_NAME, F_OK | R_OK | W_OK);
    if (sqlite3_open(FILE_NAME, &(this->db)) != 0) {
        SYNCHRONIZED_END
        cryptoFillWithRandomBytes(passwordBuffer, passwordSize);
        SDL_free(passwordBuffer);
        return false;
    }

    if (existedEarlier) this->crypto = initFromExisted(passwordBuffer);
    else this->crypto = initNew(passwordBuffer);
    assert(streamsStatesExists());

    SDL_free(passwordBuffer);
    SYNCHRONIZED_END

    if (!this->crypto) return false;
    else return true;
}

bool databaseAddConversation(unsigned userId, const Crypto* crypto) {

}

bool databaseAddMessage(const unsigned* nullable userId, const ConversationMessage* message) {

}

bool databaseUserExists(unsigned id) {

}

List* nullable databaseGetMessages(unsigned userId, unsigned* size) {

}


void databaseClean(void) {
    assert(this && !sqlite3_close(this->db));
    if (this->crypto) cryptoDestroy(this->crypto);
    SDL_DestroyMutex(this->mutex);
    SDL_free(this);
}
