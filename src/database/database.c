
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

STATIC_CONST_STRING SERVICE_TABLE = "service";
STATIC_CONST_STRING STREAMS_STATES_COLUMN = "streamsStates";
STATIC_CONST_STRING CONVERSATIONS_TABLE = "conversations";
STATIC_CONST_STRING USER_COLUMN = "user";
STATIC_CONST_STRING MESSAGES_TABLE = "messages";
STATIC_CONST_STRING TIMESTAMP_COLUMN = "timestamp";
STATIC_CONST_STRING FROM_COLUMN = "\"from\"";
STATIC_CONST_STRING TEXT_COLUMN = "text";
STATIC_CONST_STRING SIZE_COLUMN = "size";

THIS(
    SDL_mutex* mutex;
    unsigned passwordSize;
    unsigned usernameSize;
    unsigned maxMessageTextSize;
    sqlite3* db;
    Crypto* crypto;
)

typedef void (*StatementProcessor)(void* nullable, sqlite3_stmt*);

static void overloadable executeSingle(
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

static inline void overloadable executeSingle(const char* sql, unsigned sqlSize)
{ executeSingle(sql, sqlSize, NULL, NULL, NULL, NULL); }

static void createServiceTable(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table %s (%s blob(%u) not null)",
        SERVICE_TABLE, STREAMS_STATES_COLUMN, cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE)
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(sql, sqlSize);
}

static void createConversationsTable(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table %s ("
            "%s unsigned int not null unique, " // user (id)
            "%s blob(%u) not null" // streamsStates
        ")",
        CONVERSATIONS_TABLE, USER_COLUMN, STREAMS_STATES_COLUMN, cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE)
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(sql, sqlSize);
}

static void createMessagesTable(void) {
    const unsigned bufferSize = 255;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table %s ("
            "%s unsigned bigint not null, " // timestamp
            "%s unsigned int null, " // from
            "%s blob not null, " // text
            "%s unsigned int not null, " // size
            "constraint pk_%s primary key (%s, %s)" // messages, timestamp, from
        ")",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN, TEXT_COLUMN, SIZE_COLUMN,
        MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(sql, sqlSize);
}

static void createTables(void) {
    createServiceTable();
    createConversationsTable();
    createMessagesTable();
}

static void existsResultHandler(bool* result, sqlite3_stmt* statement) {
    assert(sqlite3_step(statement) == SQLITE_ROW);
    *result = sqlite3_column_int(statement, 0) > 0;
}

static bool streamsStatesExists(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select exists(select 1 from %s limit 1)",
        SERVICE_TABLE
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    bool result = false;
    executeSingle(sql, sqlSize, NULL, NULL, (StatementProcessor) &existsResultHandler, &result);
    return result;
}

static void insertEncryptedStreamStatesBinder(const byte* encryptedStreamsStates, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_blob(statement, 1, encryptedStreamsStates, cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), SQLITE_STATIC)); }

static void insertEncryptedStreamsStates(const byte* encryptedStreamsStates) {
    assert(!streamsStatesExists());

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "insert into %s (%s) values (?)",
        SERVICE_TABLE, STREAMS_STATES_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &insertEncryptedStreamStatesBinder,
        (void*) encryptedStreamsStates,
        NULL, NULL
    );
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
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select %s from %s",
        STREAMS_STATES_COLUMN, SERVICE_TABLE
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

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

bool databaseInit(byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize, unsigned maxMessageTextSize) {
    assert(!this && passwordSize > 0 && usernameSize > 0);
    this = SDL_malloc(sizeof *this);
    this->mutex = SDL_CreateMutex();
    SYNCHRONIZED_BEGIN
    this->passwordSize = passwordSize;
    this->usernameSize = usernameSize;
    this->maxMessageTextSize = maxMessageTextSize;

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

static void conversationExistsBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

bool databaseConversationExists(unsigned userId) {
    assert(this);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select exists(select 1 from %s where %s = ? limit 1)",
        CONVERSATIONS_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    bool result = false;
    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &conversationExistsBinder, &userId,
        (StatementProcessor) &existsResultHandler, &result
    );
    return result;
}

static void addConversationBinder(const void* const* parameters, sqlite3_stmt* statement) {
    assert(!sqlite3_bind_int(statement, 1, (int) *((const unsigned*) parameters[0])));
    assert(!sqlite3_bind_blob(statement, 2, (const byte*) parameters[1], cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), SQLITE_STATIC));
}

bool databaseAddConversation(unsigned userId, const Crypto* crypto) {
    assert(this);
    if (databaseConversationExists(userId)) return false;
    SYNCHRONIZED_BEGIN

    byte* streamStates = cryptoExportStreamsStates(crypto);
    byte* encryptedStreamsStates = cryptoEncrypt(this->crypto, streamStates, CRYPTO_STREAMS_STATES_SIZE, false);
    SDL_free(streamStates);
    assert(encryptedStreamsStates);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "insert into %s (%s, %s) values (?, ?)",
        CONVERSATIONS_TABLE, USER_COLUMN, STREAMS_STATES_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &addConversationBinder,
        (void*) (const void*[2]) {&userId, encryptedStreamsStates},
        NULL, NULL
    );

    SDL_free(encryptedStreamsStates);
    SYNCHRONIZED_END
    return true;
}

static void messageExistsBinder(const void* const* parameters, sqlite3_stmt* statement) {
    const unsigned* from = parameters[1];
    assert(!sqlite3_bind_int64(statement, 1, (long) *((const unsigned long*) parameters[0])));
    assert(!(from ? sqlite3_bind_int(statement, 2, (int) *from) : sqlite3_bind_null(statement, 2)));
}

bool databaseMessageExists(unsigned long timestamp, const unsigned* nullable from) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select exists(select 1 from %s where %s = ? and %s = ? limit 1)",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    bool result = false;
    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &messageExistsBinder, (const void*[2]) {&timestamp, from},
        (StatementProcessor) &existsResultHandler, &result
    );

    return result;
}

static void addMessageBinder(const void* const* parameters, sqlite3_stmt* statement) {
    const unsigned* nullable fromUserId = parameters[0];
    const ConversationMessage* message = parameters[1];
    byte* encryptedText = cryptoEncrypt(this->crypto, (byte*) message->text, message->size, false);

    assert(!sqlite3_bind_int64(statement, 1, (long) message->timestamp));
    assert(!(fromUserId ? sqlite3_bind_int(statement, 2, *(fromUserId)) : sqlite3_bind_null(statement, 2)));
    assert(!sqlite3_bind_blob(statement, 3, encryptedText, cryptoSingleEncryptedSize(message->size), SQLITE_STATIC));
    assert(!sqlite3_bind_int(statement, 4, (int) message->size));

    SDL_free(encryptedText);
}

bool databaseAddMessage(const unsigned* nullable fromUserId, const ConversationMessage* message) {
    assert(this);
    if (databaseMessageExists(message->timestamp, fromUserId)) return false;

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "insert into %s (%s, %s, %s, %s) values (?, ?, ?, ?)",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN, TEXT_COLUMN, SIZE_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &addMessageBinder, (const void*[2]) {fromUserId, message},
        NULL, NULL
    );
    return true;
}

static void getMessagesBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

static void getMessagesResultHandler(List* messages, sqlite3_stmt* statement) { // List* <ConversationMessage*>
    ConversationMessage* message; // TODO: test all
    int result;
    byte* from;
    unsigned fromId, encryptedTextSize, textSize;
    const byte* encryptedText;
    byte* text;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {

        if (sqlite3_column_type(statement, 1) != SQLITE_NULL) {
            fromId = sqlite3_column_int(statement, 1);
            from = SDL_malloc(sizeof fromId);
            SDL_memcpy(message->from, &fromId, sizeof fromId);
        } else
            from = NULL;

        encryptedTextSize = sqlite3_column_bytes(statement, 2);
        assert(encryptedTextSize >= cryptoSingleEncryptedSize(0));
        encryptedText = sqlite3_column_blob(statement, 2);

        text = cryptoDecrypt(this->crypto, encryptedText, encryptedTextSize, false);
        textSize = (unsigned) sqlite3_column_int(statement, 3);
        assert(textSize <= this->maxMessageTextSize);

        message = conversationMessageCreate(
            (unsigned long) sqlite3_column_int64(statement, 0),
            (char*) from,
            sizeof fromId,
            (char*) text,
            textSize
        );
        listAdd(messages, message);

        SDL_free(text);
        SDL_free(from);
    }

    assert(result == SQLITE_DONE);
}

List* nullable databaseGetMessages(unsigned userId, unsigned* size) {
    assert(this);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select * from %s where %s = ?",
        MESSAGES_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    List* messages = listInit((ListDeallocator) &conversationMessageDestroy);
    executeSingle(
        sql, sqlSize,
        (StatementProcessor) getMessagesBinder, &userId,
        (StatementProcessor) &getMessagesResultHandler, messages
    );

    return listSize(messages) ? messages : NULL;
}

void databaseClean(void) {
    assert(this);
    SYNCHRONIZED_BEGIN
    assert(!sqlite3_close(this->db));
    if (this->crypto) cryptoDestroy(this->crypto);
    SYNCHRONIZED_END
    SDL_DestroyMutex(this->mutex);
    SDL_free(this);
}
