/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

//#include "sqlite3proxy.h" // precompiled version of this header is included automatically by cmake
#include <SDL_stdinc.h>
#include <assert.h>
#include "../utils/rwMutex.h"
#include "database.h"

STATIC_CONST_STRING FILE_NAME = "./database.sqlite3";

STATIC_CONST_STRING CONVERSATIONS_TABLE = "conversations";
STATIC_CONST_STRING USER_COLUMN = "user";
STATIC_CONST_STRING STREAMS_STATES_COLUMN = "streamsStates";
STATIC_CONST_STRING MESSAGES_TABLE = "messages";
STATIC_CONST_STRING TIMESTAMP_COLUMN = "timestamp";
STATIC_CONST_STRING CONVERSATION_COLUMN = "conversation";
STATIC_CONST_STRING FROM_COLUMN = "\"from\""; // quotes are used 'cause it's a reserved keyword in sql
STATIC_CONST_STRING TEXT_COLUMN = "text";
STATIC_CONST_STRING SIZE_COLUMN = "size";
STATIC_CONST_STRING SERVICE_TABLE = "service";
STATIC_CONST_STRING MACHINE_ID_COLUMN = "machineId";

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    RWMutex* rwMutex;
    DatabaseHostIdSupplier hostIdSupplier;
    byte* key;
    unsigned maxMessageTextSize;
    sqlite3* db;
)
#pragma clang diagnostic pop

typedef void (*StatementProcessor)(void* nullable, sqlite3_stmt*);

struct DatabaseMessage_t {
    unsigned long timestamp;
    unsigned conversation;
    unsigned from;
    byte* text;
    unsigned size;
};

DatabaseMessage* databaseMessageCreate( // from (user id) null if from this client, the name of the sender otherwise; size of text, whereas size of from is known to all users of this api
    unsigned long timestamp,
    unsigned conversation,
    unsigned from,
    const byte* text,
    unsigned size
) {
    DatabaseMessage* message = SDL_malloc(sizeof *message);
    message->timestamp = timestamp;
    message->conversation = conversation;
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

unsigned databaseMessageConversation(const DatabaseMessage* message) {
    assert(message);
    return message->conversation;
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
    const unsigned bufferSize = (1 << 9) - 1; // 511
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create table if not exists %s (" // messages
            "%s unsigned bigint not null, " // timestamp
            "%s unsigned int not null, " // conversation
            "%s unsigned int not null, " // from
            "%s blob not null, " // text
            "%s unsigned int not null, " // size
            "primary key (%s, %s, %s), " // conversation, timestamp, from
            "foreign key (%s) references %s(%s)" // conversation, conversations, user
        ")",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, CONVERSATION_COLUMN, FROM_COLUMN, TEXT_COLUMN, SIZE_COLUMN,
        CONVERSATION_COLUMN, TIMESTAMP_COLUMN, FROM_COLUMN, CONVERSATION_COLUMN, CONVERSATIONS_TABLE, USER_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createServiceTableIfNotExists(void) {
    const unsigned bufferSize = 0xff;
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
    const unsigned bufferSize = 0xff;
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
    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "create unique index if not exists index_%s on %s(%s, %s, %s)",
        MESSAGES_TABLE, MESSAGES_TABLE, CONVERSATION_COLUMN, TIMESTAMP_COLUMN, FROM_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingleMinimal(sql, sqlSize);
}

static void createTablesIfNotExist(void) {
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

static long* nullable getMachineId(bool* exists) {
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
    *exists = true;

    byte* id = cryptoDecryptSingle(this->key, encryptedId, cryptoSingleEncryptedSize(sizeof(long)));
    SDL_free(encryptedId);

    return (long*) id;
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

    const long id = (*(this->hostIdSupplier))();
    byte* encryptedId = cryptoEncryptSingle(this->key, (const byte*) &id, sizeof(long));

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &setMachineIdBinder, encryptedId,
        NULL, NULL
    );

    SDL_free(encryptedId);
}

static bool checkKey(void) {
    bool machineIdAlreadyInserted = false;

    long* id = getMachineId(&machineIdAlreadyInserted);
    if (!machineIdAlreadyInserted) {
        assert(!id);
        setMachineId();
        return true;
    }

    bool checked = id ? *id == (*(this->hostIdSupplier))() : false;
    SDL_free(id);
    return checked;
}

bool databaseInit(
    const byte* passwordBuffer,
    unsigned passwordSize,
    unsigned usernameSize,
    unsigned maxMessageTextSize,
    DatabaseHostIdSupplier hostIdSupplier
) {
    assert(!this && passwordSize > 0 && usernameSize > 0);

    this = SDL_malloc(sizeof *this);
    this->rwMutex = rwMutexInit();
    rwMutexWriteLock(this->rwMutex);
    this->hostIdSupplier = hostIdSupplier;

    this->key = SDL_malloc(CRYPTO_KEY_SIZE);
    byte* key = cryptoMakeKey(passwordBuffer, passwordSize);
    SDL_memcpy(this->key, key, CRYPTO_KEY_SIZE);
    cryptoFillWithRandomBytes(key, CRYPTO_KEY_SIZE);
    SDL_free(key);

    this->maxMessageTextSize = maxMessageTextSize;

    bool successful = !sqlite3_open(FILE_NAME, &(this->db));
    if (successful) createTablesIfNotExist();

    if (!checkKey()) successful = false;
    rwMutexWriteUnlock(this->rwMutex);
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
    rwMutexReadLock(this->rwMutex);

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

    rwMutexReadUnlock(this->rwMutex);
    return result;
}

static void addConversationBinder(const void* const* parameters, sqlite3_stmt* statement) {
    assert(!sqlite3_bind_int(statement, 1, (int) *((const unsigned*) parameters[0])));
    assert(!sqlite3_bind_blob(statement, 2, (const byte*) parameters[1], cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE), SQLITE_STATIC));
}

bool databaseAddConversation(unsigned userId, const Crypto* crypto) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

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
    rwMutexWriteUnlock(this->rwMutex);
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
    rwMutexReadLock(this->rwMutex);

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

    if (!found) {
        rwMutexReadUnlock(this->rwMutex);
        return NULL;
    }

    byte* decryptedStreamsStates = cryptoDecryptSingle(this->key, encryptedStreamsStates, cryptoSingleEncryptedSize(CRYPTO_STREAMS_STATES_SIZE));
    assert(decryptedStreamsStates);

    Crypto* crypto = cryptoInit();
    cryptoSetUpAutonomous(crypto, this->key, decryptedStreamsStates);
    SDL_free(decryptedStreamsStates);

    rwMutexReadUnlock(this->rwMutex);
    return crypto;
}

static void removeConversationBinder(const unsigned* userId, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *userId)); }

void databaseRemoveConversation(unsigned userId) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

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

    rwMutexWriteUnlock(this->rwMutex);
}

static void addMessageBinder(const void* const* parameters, sqlite3_stmt* statement) {
    const DatabaseMessage* message = parameters[0];
    const byte* encryptedText = parameters[1];

    assert(!sqlite3_bind_int64(statement, 1, (long) message->timestamp));
    assert(!sqlite3_bind_int(statement, 2, (int) message->conversation));
    assert(!sqlite3_bind_int(statement, 3, (int) message->from));
    assert(!sqlite3_bind_blob(statement, 4, encryptedText, cryptoSingleEncryptedSize(message->size), SQLITE_STATIC));
    assert(!sqlite3_bind_int(statement, 5, (int) message->size));
}

bool databaseAddMessage(const DatabaseMessage* message) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "insert into %s (%s, %s, %s, %s, %s) values (?, ?, ?, ?, ?)",
        MESSAGES_TABLE, TIMESTAMP_COLUMN, CONVERSATION_COLUMN, FROM_COLUMN, TEXT_COLUMN, SIZE_COLUMN
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

    rwMutexWriteUnlock(this->rwMutex);
    return true;
}

static void getMessagesBinder(const unsigned* conversation, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *conversation)); }

static void getMessagesResultHandler(List* messages, sqlite3_stmt* statement) { // List* <DatabaseMessage*>
    DatabaseMessage* message;
    int result;
    unsigned encryptedTextSize, textSize;
    const byte* encryptedText;
    byte* text;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        encryptedText = sqlite3_column_blob(statement, 3);
        encryptedTextSize = sqlite3_column_bytes(statement, 3);
        assert(encryptedTextSize > cryptoSingleEncryptedSize(0));

        text = cryptoDecryptSingle(this->key, encryptedText, encryptedTextSize);
        assert(text);
        textSize = (unsigned) sqlite3_column_int(statement, 4);
        assert(encryptedTextSize == cryptoSingleEncryptedSize(textSize) && textSize <= this->maxMessageTextSize);

        message = databaseMessageCreate(
            (unsigned long) sqlite3_column_int64(statement, 0),
            (unsigned) sqlite3_column_int(statement, 1),
            (unsigned) sqlite3_column_int(statement, 2),
            text,
            textSize
        );
        listAddBack(messages, message);

        SDL_free(text);
    }

    assert(result == SQLITE_DONE);
}

List* nullable databaseGetMessages(unsigned conversation) {
    assert(this);
    rwMutexReadLock(this->rwMutex);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select * from %s where %s = ? order by %s desc",
        MESSAGES_TABLE, CONVERSATION_COLUMN, TIMESTAMP_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    List* messages = listInit((ListDeallocator) &databaseMessageDestroy);
    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &getMessagesBinder, &conversation,
        (StatementProcessor) &getMessagesResultHandler, messages
    );

    rwMutexReadUnlock(this->rwMutex);
    if (listSize(messages))
        return messages;
    else {
        listDestroy(messages);
        return NULL;
    }
}

static void removeMessagesBinder(const unsigned* conversation, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *conversation)); }

void databaseRemoveMessages(unsigned conversation) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "delete from %s where %s = ?",
        MESSAGES_TABLE, CONVERSATION_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &removeMessagesBinder, &conversation,
        NULL, NULL
    );

    rwMutexWriteUnlock(this->rwMutex);
}

static void getMostRecentMessageTimestampBinder(const unsigned* conversation, sqlite3_stmt* statement)
{ assert(!sqlite3_bind_int(statement, 1, (int) *conversation)); }

static void getMostRecentMessageTimestampResultHandler(unsigned long* timestamp, sqlite3_stmt* statement) {
    const int result = sqlite3_step(statement);
    if (result == SQLITE_ROW)
        *timestamp = (unsigned long) sqlite3_column_int64(statement, 0);
    else if (result == SQLITE_DONE)
        *timestamp = 0;
    else
        assert(false);
}

unsigned long databaseGetMostRecentMessageTimestamp(unsigned conversation) {
    assert(this);
    rwMutexReadLock(this->rwMutex);

    const unsigned bufferSize = 0xff;
    char sql[bufferSize];

    const unsigned sqlSize = (unsigned) SDL_snprintf(
        sql, bufferSize,
        "select %s from %s where %s = ? order by %s desc limit 1",
        TIMESTAMP_COLUMN, MESSAGES_TABLE, CONVERSATION_COLUMN, TIMESTAMP_COLUMN
    );
    assert(sqlSize > 0 && sqlSize <= bufferSize);

    unsigned long timestamp = 0;
    executeSingle(
        sql, sqlSize,
        (StatementProcessor) &getMostRecentMessageTimestampBinder, &conversation,
        (StatementProcessor) &getMostRecentMessageTimestampResultHandler, &timestamp
    );

    rwMutexReadUnlock(this->rwMutex);
    return timestamp;
}

void databaseClean(void) {
    assert(this);
    rwMutexWriteLock(this->rwMutex);

    cryptoFillWithRandomBytes(this->key, CRYPTO_KEY_SIZE);
    SDL_free(this->key);
    assert(!sqlite3_close(this->db));

    rwMutexWriteUnlock(this->rwMutex);
    rwMutexDestroy(this->rwMutex);
    SDL_free(this);
    this = NULL;
}
