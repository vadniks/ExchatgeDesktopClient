
//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <sdl/SDL_stdinc.h>
#include <sdl/SDL_mutex.h>
#include <assert.h>
#include <unistd.h>
#include "database.h"

#define SYNCHRONIZED_BEGIN assert(!SDL_LockMutex(this->mutex));
#define SYNCHRONIZED_END assert(!SDL_UnlockMutex(this->mutex));
#define SYNCHRONIZED(x) SYNCHRONIZED_BEGIN x SYNCHRONIZED_END

STATIC_CONST_STRING FILE_NAME = "./database.sqlite3";

STATIC_CONST_STRING CONVERSATIONS_TABLE = "conversations";
STATIC_CONST_STRING USER_COLUMN = "user";
STATIC_CONST_STRING STREAMS_STATES_COLUMN = "streamsStates";
STATIC_CONST_STRING MESSAGES_TABLE = "messages";
STATIC_CONST_STRING TIMESTAMP_COLUMN = "timestamp";
STATIC_CONST_STRING FROM_COLUMN = "\"from\""; // quotes are used 'cause it's a reserved keyword in the sql lang
STATIC_CONST_STRING TEXT_COLUMN = "text";
STATIC_CONST_STRING SIZE_COLUMN = "size";
STATIC_CONST_STRING SERVICE_TABLE = "service";
STATIC_CONST_STRING MACHINE_ID_COLUMN = "machineId";

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    SDL_mutex* mutex;
    byte* key;
    unsigned maxMessageTextSize;
    sqlite3* db;
)
#pragma clang diagnostic pop

typedef void (*StatementProcessor)(void* nullable, sqlite3_stmt*);

struct DatabaseMessage_t {
    unsigned long timestamp;
    unsigned from;
    byte* text;
    unsigned size;
};

DatabaseMessage* databaseMessageCreate( // from (user id) null if from this client, the name of the sender otherwise; size of text, whereas size of from is known to all users of this api
    unsigned long timestamp,
    unsigned from,
    const byte* text,
    unsigned size
) {
    DatabaseMessage* message = SDL_malloc(sizeof *message);
    message->timestamp = timestamp;
    message->from = from;

    message->text = SDL_malloc(size);
    SDL_memcpy(message->text, text, size);

    message->size = size;
    return message;
}

unsigned long databaseMessageTimestamp(const DatabaseMessage* message) {
    assert(message);
    return message->timestamp;
}

unsigned databaseMessageFrom(const DatabaseMessage* message) {
    assert(message);
    return message->from;
}

const byte* databaseMessageText(const DatabaseMessage* message) {
    assert(message);
    return message->text;
}

unsigned databaseMessageSize(const DatabaseMessage* message) {
    assert(message);
    return message->size;
}

void databaseMessageDestroy(DatabaseMessage* message) {
    assert(message);
    SDL_free(message->text);
    SDL_free(message);
}

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

static inline void executeSingleMinimal(const char* sql, unsigned sqlSize)
{ executeSingle(sql, sqlSize, NULL, NULL, NULL, NULL); }

static void disableJournalingResultHandler(__attribute_maybe_unused__ void* _, sqlite3_stmt* statement)
{ assert(sqlite3_step(statement) == SQLITE_ROW); }

static void disableJournaling(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "pragma journal_mode = off"
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(
        sql, sqlSize,
        NULL, NULL,
        &disableJournalingResultHandler, NULL
    );
}

static void createConversationsTableIfNotExists(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table if not exists %s ("
            "%s unsigned int not null unique, " // user (id)
            "%s blob(%u) not null, " // streamsStates
            "primary key (%s)" // user
        ")",
        CONVERSATIONS_TABLE, USER_COLUMN, STREAMS_STATES_COLUMN,
        cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createMessagesTableIfNotExits(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table if not exists %s ("
            "%s unsigned bigint not null, " // timestamp
            "%s unsigned int not null, " // from
            "%s blob not null, " // text
            "%s unsigned int not null, " // size
            "primary key (%s, %s), " // timestamp, from
            "foreign key (%s) references %s(%s)" // from, conversations, user
        ")",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN, TEXT_COLUMN, SIZE_COLUMN,
        TIMESTAMP_COLUMN, FROM_COLUMN, FROM_COLUMN, CONVERSATIONS_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createServiceTableIfNotExists(void) {
    const unsigned bufferSize = 255;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table if not exists %s(%s blob(%u))",
        SERVICE_TABLE, MACHINE_ID_COLUMN, cryptoSingleEncryptedSize(sizeof(long))
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createConversationsIndexIfNotExists(void) {
    const unsigned bufferSize = 255;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create unique index if not exists index_%s on %s(%s)",
        CONVERSATIONS_TABLE, CONVERSATIONS_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createMessagesIndexIfNotExists(void) {
    const unsigned bufferSize = 255;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create unique index if not exists index_%s on %s(%s, %s)",
        MESSAGES_TABLE, MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createTablesIfNotExists(void) {
    disableJournaling();

    createConversationsTableIfNotExists();
    createMessagesTableIfNotExits();
    createServiceTableIfNotExists();

    createConversationsIndexIfNotExists();
    createMessagesIndexIfNotExists();
}

static void getMachineIdResultHandler(byte* nullable* encryptedId, sqlite3_stmt* statement) {
    const int result = sqlite3_step(statement);

    if (result == SQLITE_ROW) {
        const unsigned size = cryptoSingleEncryptedSize(sizeof(long));
        assert((unsigned) sqlite3_column_bytes(statement, 0) == size);

        *encryptedId = SDL_malloc(size);
        SDL_memcpy(*encryptedId, sqlite3_column_blob(statement, 0), size);
    } else if (result == SQLITE_DONE)
        *encryptedId = NULL;
    else
        assert(false);
}

static long* nullable getMachineId(void) {
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select %s from %s",
        MACHINE_ID_COLUMN, SERVICE_TABLE
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    byte* encryptedId = NULL;
    executeSingle(
        sql, sqlSize,
        NULL, NULL,
        (StatementProcessor) &getMachineIdResultHandler, &encryptedId
    );
    if (!encryptedId) return NULL;

    byte* id = cryptoDecryptSingle(this->key, encryptedId, cryptoSingleEncryptedSize(sizeof(long)));
    SDL_free(encryptedId);

    return (long*) id;
}

static bool checkKey(void) {
    long* id = getMachineId();
    if (!id) return true;

    bool checked = *id == gethostid();
    SDL_free(id);
    return checked;
}

static void setMachineIdBinder(const byte* encryptedId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_blob(statement, 1, encryptedId, (int) cryptoSingleEncryptedSize(sizeof(long)), SQLITE_STATIC)); }

static void setMachineId(void) {
    const unsigned bufferSize = 255;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "insert into %s (%s) values (?)",
        SERVICE_TABLE, MACHINE_ID_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    const long id = gethostid(); // check out: man machine-id, man gethostid, cat /etc/machine-id, cat /var/lib/dbus/machine-id
    byte* encryptedId = cryptoEncryptSingle(this->key, (const byte*) &id, sizeof(long));

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &setMachineIdBinder, encryptedId,
        NULL, NULL
    );

    SDL_free(encryptedId);
}

bool databaseInit(const byte* passwordBuffer, unsigned passwordSize, unsigned usernameSize, unsigned maxMessageTextSize) {
    assert(!this && passwordSize > 0 && usernameSize > 0);
    this = SDL_malloc(sizeof *this);
    this->mutex = SDL_CreateMutex();
    SYNCHRONIZED_BEGIN

    this->key = SDL_malloc(CRYPTO_KEY_SIZE);
    byte* key = cryptoMakeKey(passwordBuffer, passwordSize);
    SDL_memcpy(this->key, key, CRYPTO_KEY_SIZE);
    SDL_free(key);

    this->maxMessageTextSize = maxMessageTextSize;

    bool successful = !sqlite3_open(FILE_NAME, &(this->db));
    if (successful) createTablesIfNotExists();

    long* machineId = getMachineId();
    if (!machineId) setMachineId();
    SDL_free(machineId);

    if (!checkKey()) successful = false;
    SYNCHRONIZED_END
    return successful;
}

static void existsResultHandler(bool* result, sqlite3_stmt* statement) {
    assert(sqlite3_step(statement) == SQLITE_ROW);
    *result = sqlite3_column_int(statement, 0) > 0;
}

static void conversationExistsBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

bool databaseConversationExists(unsigned userId) {
    assert(this);
    SYNCHRONIZED_BEGIN

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

    SYNCHRONIZED_END
    return result;
}

static void addConversationBinder(const void* const* parameters, sqlite3_stmt* statement) {
    assert(!sqlite3_bind_int(statement, 1, (int) *((const unsigned*) parameters[0])));
    assert(!sqlite3_bind_blob(statement, 2, (const byte*) parameters[1], cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), SQLITE_STATIC));
}

bool databaseAddConversation(unsigned userId, const Crypto* crypto) {
    assert(this);
    SYNCHRONIZED_BEGIN

    byte* streamStates = cryptoExportStreamsStates(crypto);
    byte* encryptedStreamsStates = cryptoEncryptSingle(this->key, streamStates, CRYPTO_STREAMS_STATES_SIZE);
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

static void getConversationBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

static void getConversationResultHandler(void* const* parameters, sqlite3_stmt* statement) {
    byte* const encryptedStreamsStates = parameters[0];
    bool* const found = parameters[1];

    const int result = sqlite3_step(statement);
    if (result == SQLITE_ROW) {
        const unsigned size = cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE);

        SDL_memcpy(encryptedStreamsStates, sqlite3_column_blob(statement, 0), size);
        assert((unsigned) sqlite3_column_bytes(statement, 0) == size);

        *found = true;
    } else if (result == SQLITE_DONE)
        *found = false;
    else
        assert(false);
}

Crypto* nullable databaseGetConversation(unsigned userId) {
    assert(this);
    SYNCHRONIZED_BEGIN

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select %s from %s where %s = ?",
        STREAMS_STATES_COLUMN, CONVERSATIONS_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    byte encryptedStreamsStates[CRYPTO_STREAMS_STATES_SIZE];
    bool found = false;

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &getConversationBinder, &userId,
        (StatementProcessor) &getConversationResultHandler, (void*[2]) {encryptedStreamsStates, &found}
    );
    if (!found) return NULL;

    byte* decryptedStreamsStates = cryptoDecryptSingle(this->key, encryptedStreamsStates, cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE));
    assert(decryptedStreamsStates);

    Crypto* crypto = cryptoInit();
    cryptoSetUpAutonomous(crypto, this->key, decryptedStreamsStates);
    SDL_free(decryptedStreamsStates);

    SYNCHRONIZED_END
    return crypto;
}

static void removeConversationBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

void databaseRemoveConversation(unsigned userId) {
    assert(this);
    SYNCHRONIZED_BEGIN

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "delete from %s where %s = ?",
        CONVERSATIONS_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &removeConversationBinder, &userId,
        NULL, NULL
    );

    SYNCHRONIZED_END
}

static void messageExistsBinder(const void* const* parameters, sqlite3_stmt* statement) {
    assert(!sqlite3_bind_int64(statement, 1, (long) *((const unsigned long*) parameters[0])));
    assert(!(sqlite3_bind_int(statement, 2, (int) *((const unsigned*) parameters[1]))));
}

bool databaseMessageExists(unsigned long timestamp, unsigned from) {
    assert(this);
    SYNCHRONIZED_BEGIN

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
        (StatementProcessor) &messageExistsBinder, (const void*[2]) {&timestamp, &from},
        (StatementProcessor) &existsResultHandler, &result
    );

    SYNCHRONIZED_END
    return result;
}

static void addMessageBinder(const void* const* parameters, sqlite3_stmt* statement) {
    const DatabaseMessage* message = parameters[0];
    const byte* encryptedText = parameters[1];

    assert(!sqlite3_bind_int64(statement, 1, (long) message->timestamp));
    assert(!sqlite3_bind_int(statement, 2, message->from));
    assert(!sqlite3_bind_blob(statement, 3, encryptedText, cryptoSingleEncryptedSize(message->size), SQLITE_STATIC));
    assert(!sqlite3_bind_int(statement, 4, (int) message->size));
}

bool databaseAddMessage(const DatabaseMessage* message) {
    assert(this);
    SYNCHRONIZED_BEGIN

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "insert into %s (%s, %s, %s, %s) values (?, ?, ?, ?)",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, FROM_COLUMN, TEXT_COLUMN, SIZE_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    byte* encryptedText = cryptoEncryptSingle(this->key, (byte*) message->text, message->size);
    assert(encryptedText);

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &addMessageBinder, (const void*[2]) {message, encryptedText},
        NULL, NULL
    );
    SDL_free(encryptedText);

    SYNCHRONIZED_END
    return true;
}

static void getMessagesBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

static void getMessagesResultHandler(List* messages, sqlite3_stmt* statement) { // List* <DatabaseMessage*>
    DatabaseMessage* message;
    int result;
    unsigned encryptedTextSize, textSize;
    const byte* encryptedText;
    byte* text;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        encryptedText = sqlite3_column_blob(statement, 2);
        encryptedTextSize = sqlite3_column_bytes(statement, 2);
        assert(encryptedTextSize > cryptoSingleEncryptedSize(0));

        text = cryptoDecryptSingle(this->key, encryptedText, encryptedTextSize);
        assert(text);
        textSize = (unsigned) sqlite3_column_int(statement, 3);
        assert(encryptedTextSize == cryptoSingleEncryptedSize(textSize) && textSize <= this->maxMessageTextSize);

        message = databaseMessageCreate(
            (unsigned long) sqlite3_column_int64(statement, 0),
            sqlite3_column_int(statement, 1),
            text,
            textSize
        );
        listAddBack(messages, message);

        SDL_free(text);
    }

    assert(result == SQLITE_DONE);
}

List* nullable databaseGetMessages(unsigned userId) {
    assert(this);
    SYNCHRONIZED_BEGIN

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select * from %s where %s = ? order by %s desc",
        MESSAGES_TABLE, FROM_COLUMN, TIMESTAMP_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    List* messages = listInit((ListDeallocator) &databaseMessageDestroy);
    executeSingle(
        sql, sqlSize,
        (StatementProcessor) getMessagesBinder, &userId,
        (StatementProcessor) &getMessagesResultHandler, messages
    );

    SYNCHRONIZED_END
    if (listSize(messages))
        return messages;
    else {
        listDestroy(messages);
        return NULL;
    }
}

void databaseClean(void) {
    assert(this);
    SYNCHRONIZED_BEGIN

    SDL_free(this->key);
    assert(!sqlite3_close(this->db));

    SYNCHRONIZED_END
    SDL_DestroyMutex(this->mutex);
    SDL_free(this);
}
