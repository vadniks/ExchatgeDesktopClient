/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)
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

#include <SDL.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "user.h"
#include "conversationMessage.h"
#include "lifecycle.h"
#include "database/database.h"
#include "options.h"
#include "collections/list.h"
#include "collections/queue.h"
#include "logic.h"

const unsigned LOGIC_MAX_FILE_PATH_SIZE = 0x1ff; // 511, (1 << 9) - 1
STATIC_CONST_UNSIGNED MAX_FILE_SIZE = (1 << 20) * 20; // 1024^2 * 20 = 20971520 bytes = 20 mb

STATIC_CONST_STRING FILES_DIR = "files";

typedef enum : unsigned {
    STATE_UNAUTHENTICATED = 0,
    STATE_AWAITING_AUTHENTICATION = 1,
    STATE_AUTHENTICATED = 2,
    STATE_EXCHANGING_MESSAGES = 3
} States;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    atomic bool netInitialized;
    List* usersList; // <User*>
    List* messagesList; // <ConversationMessage*>
    bool adminMode;
    atomic unsigned state;
    char* currentUserName;
    atomic unsigned toUserId; // the id of the user, the current user (logged in via this client) wanna speak to
    atomic bool databaseInitialized;
    SDL_RWops* nullable rwops;
    atomic unsigned fileBytesCounter;
    bool autoLoggingIn;
    void* fileHashState;
    atomic unsigned missingMessagesFetchers;
    Queue* userIdsToFetchMessagesFrom;
    OptionsThemes theme;
)
#pragma clang diagnostic pop

static long fetchHostId(void);

static void showLoginPageOrPerformAutoLoggingIn(void) {
    if (!this->autoLoggingIn) {
        renderShowLogIn();
        return;
    }

    const char* credentials = optionsCredentials();
    assert(credentials);

    logicOnCredentialsReceived(
        credentials,
        NET_USERNAME_SIZE,
        credentials + NET_USERNAME_SIZE,
        NET_UNHASHED_PASSWORD_SIZE,
        true
    );
}

void logicInit(void) {
    assert(!this);
    assert(NET_MAX_MESSAGE_BODY_SIZE >= RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE);

    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
    this->usersList = userInitList();
    this->messagesList = conversationMessageInitList();
    this->state = STATE_UNAUTHENTICATED;
    this->currentUserName = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
    this->databaseInitialized = false;
    this->rwops = NULL;
    this->fileHashState = NULL;
    this->missingMessagesFetchers = 0;
    this->userIdsToFetchMessagesFrom = queueInit(NULL);

    cryptoInit();

    assert(optionsInit(NET_USERNAME_SIZE, NET_UNHASHED_PASSWORD_SIZE, &fetchHostId));
    this->adminMode = optionsIsAdmin();
    this->autoLoggingIn = optionsCredentials() != NULL;
    this->theme = optionsTheme();

    lifecycleAsync((LifecycleAsyncActionFunction) &showLoginPageOrPerformAutoLoggingIn, NULL, 1000);
}

bool logicIsAdminMode(void) {
    assert(this);
    return this->adminMode;
}

bool logicIsDarkTheme(void) {
    assert(this);
    return this->theme;
}

void logicNetListen(void) {
    if (!this) return; // logic module might haven't been initialized by the time lifecycle module's netThread has started periodically calling this function to update connection
    if (!this->netInitialized) return;
    netListen();
}

List* logicUsersList(void) {
    assert(this);
    return this->usersList;
}

List* logicMessagesList(void) {
    assert(this);
    return this->messagesList;
}

byte logicLanguage(void) {
    assert(this);
    return optionsLanguage();
}

void logicOnAutoLoggingInChanged(bool value) {
    assert(this);

    if (this->autoLoggingIn)
        optionsSetCredentials(NULL);

    this->autoLoggingIn = value;
}

bool logicAutoLoggingInSupplier(void) {
    assert(this);
    return this->autoLoggingIn;
}

static unsigned maxUnencryptedMessageBodySize(void) { return NET_MAX_MESSAGE_BODY_SIZE - cryptoEncryptedSize(0); } // 160 - 17 = 143

static int findUserComparator(const unsigned* xId, const User* const* user)
{ return *xId < (*user)->id ? -1 : (*xId > (*user)->id ? 1 : 0); }

static const User* nullable findUser(unsigned id)
{ return listBinarySearch(this->usersList, &id, (ListComparator) &findUserComparator); } // TODO: add mutex to Crypto objects

static void processReceivedMessage(void** parameters) {
    assert(this && this->databaseInitialized);

    const unsigned long timestamp = *((unsigned long*) parameters[0]);
    const unsigned fromId = *((unsigned*) parameters[1]), encryptedSize = *((unsigned*) parameters[3]);

    byte encryptedMessage[encryptedSize];
    SDL_memcpy(encryptedMessage, parameters[2], encryptedSize);

    for (byte i = 0; i < 4; SDL_free(parameters[i++]));
    SDL_free(parameters);

    const User* user = findUser(fromId);
    if (!user) return; // thanks to caching messages while users information is synchronizing, losing messages here is unlikely to happen, but theoretically it is still possible - it would be an error though as the sender is required to instantiate a conversation with the recipient first and the recipient's logic won't let to do that as it would drop/ignore conversation creation requests/invites from the sender - TODO: assert(user found)?

    const unsigned paddedSize = encryptedSize - cryptoEncryptedSize(0);
    assert(paddedSize > 0 && paddedSize <= maxUnencryptedMessageBodySize());

    // TODO: test conversation setup and file exchanging with 3 users: 2 try to setup/exchange and the 3rd one tries to interfere

    CryptoCoderStreams* coderStreams = databaseGetConversation(fromId);
    if (!coderStreams) return; // TODO: assert

    byte* paddedMessage = cryptoDecrypt(coderStreams, encryptedMessage, encryptedSize, false);
    assert(paddedMessage); // situation: user1 updating users list & fetching messages, meanwhile user2 sends a paddedMessage, so we have the following: user1 updated all needed info and didn't receive the new paddedMessage from user2, user1 doesn't know that there's a new missing paddedMessage on a server, then without updating messages on user1's side, user2 sends him a new paddedMessage, what will happen next? - user1 receives the second new paddedMessage from user2 and tries to decrypt it - it may fail due to 'ratchet' mechanism in the stream cipher. Besides, if user1 after receiving the second paddedMessage will try to re-fetch messages, he won't receive the first missing paddedMessage - only those after the second paddedMessage
    cryptoCoderStreamsDestroy(coderStreams); // tested this situation ----/\ and everything works fine
    // TODO: to avoid possibility of this problem appearing can be implemented the following mechanism: periodically check for missing messages presence (like with checking for a new paddedMessage) and update if needed

    unsigned size;
    byte* message = cryptoRemovePadding(&size, paddedMessage, paddedSize);
    assert(message && size);

    DatabaseMessage* dbMessage = databaseMessageCreate(timestamp, fromId, fromId, message, size);
    assert(databaseAddMessage(dbMessage));
    databaseMessageDestroy(dbMessage);

    if (fromId == this->toUserId)
        listAddFront(this->messagesList, conversationMessageCreate(timestamp, user->name, NET_USERNAME_SIZE, (const char*) message, size));

    SDL_free(paddedMessage);
    SDL_free(message);
}

static void onMessageReceived(unsigned long timestamp, unsigned fromId, const byte* encryptedMessage, unsigned encryptedSize) {
    void** parameters = SDL_malloc(4 * sizeof(void*));

    parameters[0] = SDL_malloc(sizeof(long));
    *((unsigned long*) parameters[0]) = timestamp;

    parameters[1] = SDL_malloc(sizeof(int));
    *((unsigned*) parameters[1]) = fromId;

    parameters[2] = SDL_malloc(encryptedSize);
    SDL_memcpy(parameters[2], encryptedMessage, encryptedSize);

    parameters[3] = SDL_malloc(sizeof(int));
    *((unsigned*) parameters[3]) = encryptedSize;

    lifecycleAsync((LifecycleAsyncActionFunction) &processReceivedMessage, parameters, 0);
}

static void beginLoading(void) {
    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);
}

static void finishLoading(void) {
    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
}

static void onLogInResult(bool successful) { // TODO: add broadcasting to all users feature for admin
    assert(this);
    if (successful) {
        this->state = STATE_AUTHENTICATED;
        lifecycleAsync((LifecycleAsyncActionFunction) &netFetchUsers, NULL, 0);
    } else {
        this->state = STATE_UNAUTHENTICATED;
        renderShowLogIn();
        renderShowSystemError();
        finishLoading();
    }
}

static void onErrorReceived(__attribute_maybe_unused__ int flag) {
    assert(this);
    finishLoading(); // TODO: apply these only for logging in/registration
    renderShowSystemError();
}

static void onRegisterResult(bool successful) {
    assert(this);
    this->state = STATE_UNAUTHENTICATED;
    successful ? renderShowRegistrationSucceededSystemMessage() : renderShowSystemError();
    renderHideInfiniteProgressBar();
}

static void onDisconnected(void) {
    assert(this);

    this->netInitialized = false;
    this->state = STATE_UNAUTHENTICATED;

    finishLoading();
    renderShowLogIn();
    renderShowDisconnectedError();
}

static void fetchMissingMessagesFromUser(unsigned id) {
    assert(this && this->databaseInitialized);
    this->missingMessagesFetchers++;

    const unsigned long timestamp = databaseGetMostRecentMessageTimestamp(id),
        minimalPossibleTimestamp = databaseGetConversationTimestamp(id) + 1;

    assert(timestamp < logicCurrentTimeMillis());
    netFetchMessages(id, timestamp < minimalPossibleTimestamp ? minimalPossibleTimestamp : timestamp);
}

static void processFetchedUsers(List* userInfosList) {
    assert(this && this->databaseInitialized && !this->missingMessagesFetchers);
    listClear(this->usersList);

    const unsigned size = listSize(userInfosList);
    const NetUserInfo* info;
    bool conversationExists;

    queueClear(this->userIdsToFetchMessagesFrom); // TODO: delete all messages on server on conversation deletion on client

    for (unsigned i = 0, id; i < size; i++) {
        info = listGet(userInfosList, i);
        id = netUserInfoId(info);

        if (id != netCurrentUserId()) {
            conversationExists = databaseConversationExists(id);

            listAddBack(this->usersList, userCreate(
                id,
                (const char*) netUserInfoName(info),
                NET_USERNAME_SIZE,
                conversationExists,
                netUserInfoConnected(info)
            ));

            if (conversationExists)
                queuePush(this->userIdsToFetchMessagesFrom, (void*) (long) id);
        } else {
            SDL_memcpy(this->currentUserName, netUserInfoName(info), NET_USERNAME_SIZE);
            renderSetWindowTitle(this->currentUserName);
        }
    }
    listDestroy(userInfosList);

    renderShowUsersList(this->currentUserName);

    if (!queueSize(this->userIdsToFetchMessagesFrom)) {
        netSetIgnoreUsualMessages(false);
        finishLoading(); // if at least one conversation exists, then begin outdated/missing messages fetching, otherwise do nothing and release locks
    } else
        fetchMissingMessagesFromUser((unsigned) (long) queuePop(this->userIdsToFetchMessagesFrom));
}

static void onUsersFetched(List* userInfosList) {
    List* xUserInfosList = listCopy(userInfosList, (ListItemDuplicator) &netUserInfoCopy);
    netSetIgnoreUsualMessages(true);
    lifecycleAsync((LifecycleAsyncActionFunction) &processFetchedUsers, xUserInfosList, 0);
}

static void fetchMissingMessagesFromUserWrapper(const void* id)
{ fetchMissingMessagesFromUser((unsigned) (long) id); }

static void onNextMessageFetched(
    unsigned from,
    unsigned long timestamp,
    unsigned size,
    const byte* nullable message,
    bool last
) {
    assert(this);
    assert(size && message || !size && !message);

    if (size > 0) onMessageReceived(timestamp, from, message, size);

    assert(this->missingMessagesFetchers);

    if (!last) return;
    this->missingMessagesFetchers--;
    assert(this->missingMessagesFetchers < 0u - 1u); // check overflow

    if (queueSize(this->userIdsToFetchMessagesFrom))
        lifecycleAsync((LifecycleAsyncActionFunction) &fetchMissingMessagesFromUserWrapper, queuePop(this->userIdsToFetchMessagesFrom), 0);

    assert(this->missingMessagesFetchers == queueSize(this->userIdsToFetchMessagesFrom));

    if (this->missingMessagesFetchers) return;
    queueClear(this->userIdsToFetchMessagesFrom);
    netSetIgnoreUsualMessages(false);
    finishLoading();
}

static void tryLoadPreviousMessages(unsigned id) {
    assert(this && this->databaseInitialized);
    listClear(this->messagesList);

    List* messages = databaseGetMessages(id);
    if (!messages) return;

    const User* user = findUser(id);
    assert(user);

    const DatabaseMessage* dbMessage;
    bool fromCurrentUser;
    for (unsigned i = 0; i < listSize(messages); i++) {
        dbMessage = listGet(messages, i);
        fromCurrentUser = databaseMessageFrom(dbMessage) == netCurrentUserId();

        listAddBack(this->messagesList, conversationMessageCreate(
            databaseMessageTimestamp(dbMessage),
            fromCurrentUser ? NULL : user->name,
            fromCurrentUser ? 0 : NET_USERNAME_SIZE,
            (const char*) databaseMessageText(dbMessage),
            databaseMessageSize(dbMessage)
        ));
    }

    listDestroy(messages);
}

static inline void updateUsersList(void) { logicOnUpdateUsersListClicked(); }

static void replyToConversationSetUpInvite(unsigned* fromId) {
    assert(this && fromId);
    unsigned xFromId = *fromId;
    SDL_free(fromId);

    listClear(this->messagesList);

    if (databaseConversationExists(xFromId))
        databaseRemoveConversation(xFromId), // existence of a conversation when receiving an invite means that user, from whom this invite came, has deleted conversation with the current user, so to make users messaging again, remove the outdated conversation on the current user's side
        databaseRemoveMessages(xFromId);

    const User* user = findUser(xFromId);
    if (!user) goto releaseLocks; // if local users list hasn't been synchronized yet

    CryptoCoderStreams* coderStreams = netReplyToConversationSetUpInvite(renderShowInviteDialog(user->name), xFromId);
    if (coderStreams)
        this->toUserId = xFromId, // not only in python there's indentation based scoping, here's an emulation though
        this->state = STATE_EXCHANGING_MESSAGES,

        assert(databaseAddConversation(xFromId, coderStreams, logicCurrentTimeMillis())),
        cryptoCoderStreamsDestroy(coderStreams),
        updateUsersList();
    else
        renderShowUnableToCreateConversation();

    releaseLocks:
    finishLoading();
}

static void onConversationSetUpInviteReceived(unsigned fromId) {
    assert(this);
    beginLoading();

    unsigned* xFromId = SDL_malloc(sizeof(int));
    *xFromId = fromId;
    lifecycleAsync((LifecycleAsyncActionFunction) &replyToConversationSetUpInvite, xFromId, 0);
}

void logicOnFileChooserRequested(void) {
    assert(this);
    renderShowFileChooser();
}

static byte* nullable calculateOpenedFileChecksum(void) {
    assert(this->rwops);

    const unsigned bufferSize = maxUnencryptedMessageBodySize();
    byte buffer[bufferSize];
    SDL_memset(buffer, 0, bufferSize);

    void* state = cryptoHashMultipart(NULL, NULL, 0);
    bool read = false;
    unsigned count;

    while ((count = SDL_RWread(this->rwops, buffer, 1, bufferSize)) > 0) {
        read = true;
        cryptoHashMultipart(state, buffer, count);
        SDL_memset(buffer, 0, bufferSize);
    }

    SDL_RWseek(this->rwops, 0, RW_SEEK_SET);

    byte* hash = NULL;
    if (!read)
        SDL_free(state);
    else
        hash = cryptoHashMultipart(state, NULL, 0);

    return hash;
}

static void beginFileExchange(void** parameters) {
    assert(this);
    this->fileBytesCounter = 0;

    const unsigned fileSize = (long) parameters[0];

    byte* hash = calculateOpenedFileChecksum();
    assert(hash);

    if (!netBeginFileExchange(
        this->toUserId,
        fileSize,
        hash,
        parameters[2],
        (long) parameters[1]
    ) || this->fileBytesCounter != fileSize) // blocks the thread until file is fully transmitted or error occurred or receiver declined the exchanging
        renderShowUnableToTransmitFileError();
    else
        renderShowFileTransmittedSystemMessage();

    SDL_free(parameters[2]);
    SDL_free(parameters);

    assert(!SDL_RWclose(this->rwops));
    this->rwops = NULL;

    SDL_free(hash);

    finishLoading();
}

static const char* nullable xBasename(const char* path) {
    const char* lastDelimiter = path;
    while (*(path++) != 0)
        if (*path == '/')
            lastDelimiter = path;

    const char* lastSliceStart = lastDelimiter + 1;
    return lastSliceStart < path && *lastSliceStart != 0 ? lastSliceStart : NULL;
}

void logicFileChooseResultHandler(const char* nullable filePath, unsigned size) {
    assert(this);
    assert(filePath || !filePath && !size);

    beginLoading();

    const User* user = findUser(this->toUserId);
    assert(user);

    if (!filePath) {
        finishLoading();
        renderShowConversation(user->name);
        return;
    }

    if (!user->online) {
        finishLoading();
        renderShowUserIsOfflineError();
        return;
    }

    if (!size) {
        finishLoading();
        renderShowEmptyFilePathError();
        return;
    }

    assert(!this->rwops);
    this->rwops = SDL_RWFromFile(filePath, "rb");

    if (!this->rwops) {
        finishLoading();
        renderShowCannotOpenFileError();
        return;
    }

    const long fileSize = SDL_RWsize(this->rwops);

    if (fileSize < 0 || !fileSize) {
        assert(!SDL_RWclose(this->rwops));
        this->rwops = NULL;

        finishLoading();
        renderShowFileIsEmptyError();
        return;
    }

    if (fileSize > MAX_FILE_SIZE) {
        assert(!SDL_RWclose(this->rwops));
        this->rwops = NULL;

        finishLoading();
        renderShowFileIsTooBig();
        return;
    }

    const char* filename = xBasename(filePath);
    assert(filename);
    const unsigned long filenameSize = SDL_strlen(filename);
    assert(filenameSize && filenameSize <= NET_MAX_FILENAME_SIZE);

    void** parameters = SDL_malloc(3 * sizeof(void*));
    parameters[0] = (void*) fileSize;
    parameters[1] = (void*) filenameSize;
    (parameters[2] = SDL_malloc(filenameSize)) && SDL_memcpy(parameters[2], filename, filenameSize);

    lifecycleAsync((LifecycleAsyncActionFunction) &beginFileExchange, parameters, 0);
}

static void replyToFileExchangeRequest(void** parameters) {
    assert(this);
    this->fileBytesCounter = 0;

    const unsigned
        fromId = *((unsigned*) parameters[0]),
        fileSize = *((unsigned*) parameters[1]),
        filenameSize = *((unsigned*) parameters[3]);

    byte originalHash[CRYPTO_HASH_SIZE];
    SDL_memcpy(originalHash, parameters[2], CRYPTO_HASH_SIZE); // TODO: excessive copy operations

    char filename[filenameSize];
    SDL_memcpy(filename, parameters[4], filenameSize);

    for (byte i = 0; i < 5; SDL_free(parameters[i++]));
    SDL_free(parameters);

    const User* user = findUser(fromId);
    if (!user) {
        finishLoading();
        return;
    }

    const bool accepted = renderShowFileExchangeRequestDialog(user->name, fileSize, filename); // blocks the thread
    assert(this);

    if (!accepted) {
        assert(!netReplyToFileExchangeInvite(fromId, fileSize, false)); // blocks the thread again
        finishLoading();
        renderShowUnableToTransmitFileError();
        return;
    }

    const unsigned filesDirSize = SDL_strlen(FILES_DIR);
    char filesDir[LOGIC_MAX_FILE_PATH_SIZE];
    SDL_memset(filesDir, 0, LOGIC_MAX_FILE_PATH_SIZE);

    assert(getcwd(filesDir, LOGIC_MAX_FILE_PATH_SIZE - filesDirSize - 1));

    const unsigned currentPathSize = SDL_strlen(filesDir);
    filesDir[currentPathSize] = '/';
    SDL_memcpy(filesDir + currentPathSize + 1, FILES_DIR, filesDirSize);

    struct stat xStat;
    if (stat(filesDir, &xStat) || !S_ISDIR(xStat.st_mode))
        assert(!mkdir(filesDir, S_IRWXU));

    char filePath[LOGIC_MAX_FILE_PATH_SIZE];
    const unsigned written = SDL_snprintf(
        filePath, LOGIC_MAX_FILE_PATH_SIZE,
        "%s/%lu_%s",
        filesDir,
        logicCurrentTimeMillis(),
        filename
    );
    assert(written > 0 && written <= LOGIC_MAX_FILE_PATH_SIZE);

    assert(!this->rwops);
    this->rwops = SDL_RWFromFile(filePath, "wb");

    if (!this->rwops) {
        assert(!netReplyToFileExchangeInvite(fromId, fileSize, false)); // blocks the thread again
        finishLoading();
        renderShowUnableToTransmitFileError();
        return;
    }

    assert(!this->fileHashState);
    const bool exchangeResult = netReplyToFileExchangeInvite(fromId, fileSize, true); // blocks the thread again
    assert(!SDL_RWclose(this->rwops));
    this->rwops = NULL;

    bool hashesEqual = false;
    if (this->fileHashState) {
        byte* hash = cryptoHashMultipart(this->fileHashState, NULL, 0);
        this->fileHashState = NULL;

        hashesEqual = !SDL_memcmp(originalHash, hash, CRYPTO_HASH_SIZE);
        SDL_free(hash);
    }

    if (!exchangeResult || this->fileBytesCounter != fileSize || !hashesEqual) {
        renderShowUnableToTransmitFileError();
        unlink(filePath);
    } else
        renderShowFileTransmittedSystemMessage();

    assert(!this->rwops);
    finishLoading();
}

static void onFileExchangeInviteReceived(
    unsigned fromId,
    unsigned fileSize,
    const byte* originalHash,
    const char* filename,
    unsigned filenameSize
) {
    assert(this);
    beginLoading();

    void** parameters = SDL_malloc(5 * sizeof(void*));
    (parameters[0] = SDL_malloc(sizeof(int))) && (*((unsigned*) parameters[0]) = fromId);
    (parameters[1] = SDL_malloc(sizeof(int))) && (*((unsigned*) parameters[1]) = fileSize);
    (parameters[2] = SDL_malloc(CRYPTO_HASH_SIZE)) && SDL_memcpy(parameters[2], originalHash, CRYPTO_HASH_SIZE);
    (parameters[3] = SDL_malloc(sizeof(int))) && (*((unsigned*) parameters[3]) = filenameSize);
    (parameters[4] = SDL_malloc(filenameSize)) && SDL_memcpy(parameters[4], filename, filenameSize);

    lifecycleAsync((LifecycleAsyncActionFunction) &replyToFileExchangeRequest, parameters, 0);
}

static unsigned nextFileChunkSupplier(unsigned index, byte* encryptedBuffer) { // TODO: notify user when a new message has been received
    assert(this && this->rwops);
    const unsigned targetSize = maxUnencryptedMessageBodySize();

    byte unencryptedBuffer[targetSize];
    const unsigned actualSize = SDL_RWread(this->rwops, unencryptedBuffer, 1, targetSize);
    if (!actualSize) goto finish;

    CryptoCoderStreams* coderStreams = databaseGetConversation(this->toUserId);
    assert(coderStreams);

    const unsigned encryptedSize = cryptoEncryptedSize(actualSize);
    assert(encryptedSize <= NET_MAX_MESSAGE_BODY_SIZE);

    byte* encryptedChunk = cryptoEncrypt(coderStreams, unencryptedBuffer, actualSize, false);
    assert(encryptedChunk);
    SDL_memcpy(encryptedBuffer, encryptedChunk, encryptedSize);
    SDL_free(encryptedChunk);

    cryptoCoderStreamsDestroy(coderStreams);

    if (!index) assert(!this->fileBytesCounter);
    this->fileBytesCounter += actualSize;
    return encryptedSize;

    finish:
    assert(index);
    // this->rwops is freed elsewhere
    return 0;
}

static void nextFileChunkReceiver(
    unsigned fromId,
    unsigned index,
    unsigned receivedBytesCount,
    const byte* encryptedBuffer
) {
    assert(this && this->rwops);

    const unsigned decryptedSize = receivedBytesCount - cryptoEncryptedSize(0);
    assert(decryptedSize && decryptedSize <= maxUnencryptedMessageBodySize());

    CryptoCoderStreams* coderStreams = databaseGetConversation(fromId);
    assert(coderStreams);

    byte* decrypted = cryptoDecrypt(coderStreams, encryptedBuffer, receivedBytesCount, false);
    assert(decrypted);

    assert(SDL_RWwrite(this->rwops, decrypted, 1, decryptedSize) == decryptedSize);

    if (!index) {
        assert(!this->fileHashState);
        this->fileHashState = cryptoHashMultipart(NULL, NULL, 0);
    } else
        assert(this->fileHashState);
    cryptoHashMultipart(this->fileHashState, decrypted, decryptedSize);

    SDL_free(decrypted);

    cryptoCoderStreamsDestroy(coderStreams);

    if (!index) assert(!this->fileBytesCounter);
    this->fileBytesCounter += decryptedSize;

    // this->fileHashState and this->rwops are freed elsewhere
}

void logicOnAdminActionsPageRequested(bool enter) {
    assert(this);
    if (enter) renderShowAdminActions();
    else renderShowUsersList(this->currentUserName);
}

void logicOnBroadcastMessageSendRequested(const char* text, unsigned size) {
    assert(this && this->netInitialized);
    assert(!size || size <= NET_MAX_MESSAGE_BODY_SIZE && size <= RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE);
    if (!size) return;
    netSendBroadcast((const byte*) text, size);
}

bool logicFilePresenceChecker(const char* file) { return !access(file, F_OK | R_OK); }

static void onBroadcastMessageReceived(const byte* text, unsigned size) {
    assert(this);
    renderShowSystemMessage((const char*) text, size);
}

static void clipboardPaste(void) {
    if (!SDL_HasClipboardText()) return;
    if (!renderIsConversationShown() && !renderIsFileChooserShown()) return;

    const unsigned maxSize = renderIsConversationShown() ? logicMaxMessagePlainPayloadSize() : LOGIC_MAX_FILE_PATH_SIZE;
    char* text = SDL_GetClipboardText(), * buffer = NULL;
    unsigned size;

    for (size = 0; text[size] && size <= maxSize; size++)
        buffer = SDL_realloc(buffer, size + 1),
        buffer[size] = text[size];

    SDL_free(text);

    if (size) {
        const unsigned totalSize = size <= maxSize ? size : maxSize;
        renderIsConversationShown()
            ? renderAlterConversationMessageBuffer(buffer, totalSize)
            : renderAlterFilePathBuffer(buffer, totalSize);
    }
    SDL_free(buffer);
}

static void clipboardCopy(void) {}

void logicProcessEvent(SDL_Event* event) {
    static bool ctrlPressed;
    if (event->key.keysym.sym == SDLK_LCTRL)
        ctrlPressed = event->type == SDL_KEYDOWN;

    const SDL_KeyCode keyCode = event->key.keysym.sym;

    if (ctrlPressed && event->type == SDL_KEYUP)
         keyCode == SDLK_v
            ? clipboardPaste()
            : keyCode == SDLK_c
                ? clipboardCopy()
                : STUB;
}

// TODO: add possibility for admin to remove users from database; to disable/enable registration; to ban/unban users; to kick connected users

static void processCredentials(void** data) {
    const char* username = data[0];
    const char* password = data[1];
    const bool logIn = *((bool*) data[2]);

    assert(this);
    if (!logIn) goto netInit;

    if (this->autoLoggingIn) {
        char credentials[NET_USERNAME_SIZE + NET_UNHASHED_PASSWORD_SIZE];
        SDL_memcpy(credentials, username, NET_USERNAME_SIZE);
        SDL_memcpy(credentials + NET_USERNAME_SIZE, password, NET_UNHASHED_PASSWORD_SIZE);
        optionsSetCredentials(credentials);
        logicCredentialsRandomFiller(credentials, sizeof credentials);
    }

    if (this->databaseInitialized) {
        databaseClean();
        this->databaseInitialized = false;
    }

    if (!databaseInit(
        (byte*) password,
        NET_UNHASHED_PASSWORD_SIZE,
        NET_USERNAME_SIZE,
        logicMaxMessagePlainPayloadSize(),
        &fetchHostId
    )) { // password that's used to sign in also used to encrypt messages & other stuff in the database
        databaseClean();
        renderShowUnableToDecryptDatabaseError();
        this->state = STATE_UNAUTHENTICATED;

        if (this->autoLoggingIn)
            renderShowLogIn();

        finishLoading();
        goto cleanup;
    } else
        this->databaseInitialized = true;

    netInit:
    this->netInitialized = netInit(
        optionsHost(),
        optionsPort(),
        optionsServerSignPublicKey(),
        optionsServerSignPublicKeySize(),
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected,
        &logicCurrentTimeMillis,
        &onUsersFetched,
        &onConversationSetUpInviteReceived,
        &onFileExchangeInviteReceived,
        &nextFileChunkSupplier,
        &nextFileChunkReceiver,
        &onNextMessageFetched,
        &onBroadcastMessageReceived
    );

    if (!this->netInitialized) {
        this->state = STATE_UNAUTHENTICATED;
        renderShowUnableToConnectToTheServerError();

        if (this->autoLoggingIn)
            renderShowLogIn();

        finishLoading();
    } else
        logIn ? netLogIn(username, password) : netRegister(username, password);

    cleanup:
    logicCredentialsRandomFiller(data[0], NET_USERNAME_SIZE);
    logicCredentialsRandomFiller(data[1], NET_UNHASHED_PASSWORD_SIZE);

    SDL_free(data[0]);
    SDL_free(data[1]);
    SDL_free(data[2]);
    SDL_free(data);
}

void logicOnCredentialsReceived(
    const char* username,
    unsigned usernameSize,
    const char* password,
    unsigned passwordSize,
    bool logIn
) {
    assert(this && usernameSize <= NET_USERNAME_SIZE && passwordSize <= NET_UNHASHED_PASSWORD_SIZE);
    if (this->state != STATE_UNAUTHENTICATED) return;
    assert(!this->netInitialized);

    this->state = STATE_AWAITING_AUTHENTICATION;
    beginLoading();

    void** data = SDL_malloc(3 * sizeof(void*));
    data[0] = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
    data[1] = SDL_calloc(NET_UNHASHED_PASSWORD_SIZE, sizeof(char));
    data[2] = SDL_malloc(sizeof(bool));

    SDL_memcpy(data[0], username, usernameSize);
    SDL_memcpy(data[1], password, passwordSize);
    *((bool*) data[2]) = logIn;

    lifecycleAsync((LifecycleAsyncActionFunction) &processCredentials, data, 0);
}

void logicCredentialsRandomFiller(char* credentials, unsigned size) {
    assert(this);
    cryptoFillWithRandomBytes((byte*) credentials, size);
}

void logicOnLoginRegisterPageQueriedByUser(bool logIn) {
    assert(this);
    logIn ? renderShowLogIn() : renderShowRegister();
}

static void startConversation(void** parameters) {
    unsigned* id = parameters[0];
    assert(this && this->databaseInitialized);

    if (databaseConversationExists(*id))
        renderShowConversationAlreadyExists();
    else {
        CryptoCoderStreams* coderStreams = NULL;

        if ((coderStreams = netCreateConversation(*id))) // blocks the thread until either an error has happened or the conversation has been created
            assert(databaseAddConversation(*id, coderStreams, logicCurrentTimeMillis())),
            cryptoCoderStreamsDestroy(coderStreams),
            updateUsersList();
        else
            renderShowUnableToCreateConversation();
    }

    SDL_free(id);
    SDL_free(parameters);

    finishLoading();
}

static void continueConversation(void** parameters) {
    unsigned* id = parameters[0];
    const User* user = parameters[1];

    assert(this && this->databaseInitialized);

    if (!databaseConversationExists(*id))
        renderShowConversationDoesntExist();
    else
        tryLoadPreviousMessages(*id),
        renderShowConversation(user->name);

    SDL_free(id);
    SDL_free(parameters);

    finishLoading();
}

static void createOrLoadConversation(unsigned id, bool create) {
    const User* user = findUser(id);
    assert(user);

    if (create && !(user->online)) {
        renderShowUserIsOfflineError();
        return;
    }

    beginLoading();

    void** parameters = SDL_malloc(2 * sizeof(void*));
    (parameters[0] = SDL_malloc(sizeof(int))) && (*((unsigned*) parameters[0]) = id);
    ((const void**) parameters)[1] = user;

    lifecycleAsync((LifecycleAsyncActionFunction) (create ? &startConversation : &continueConversation), parameters, 0); // TODO: update users list after start/continue is performed
}

static void deleteConversation(unsigned* id) {
    assert(this && this->databaseInitialized);

    if (databaseConversationExists(*id)) {
        databaseRemoveConversation(*id);
        databaseRemoveMessages(*id);
    } else
        renderShowConversationDoesntExist();

    SDL_free(id);

    finishLoading();
    updateUsersList();
}

void logicOnUserForConversationChosen(unsigned id, RenderConversationChooseVariants chooseVariant) {
    assert(this);
    this->state = STATE_EXCHANGING_MESSAGES;
    this->toUserId = id;
    listClear(this->messagesList);

    switch (chooseVariant) {
        case RENDER_CONVERSATION_START: fallthrough
        case RENDER_CONVERSATION_CONTINUE:
            createOrLoadConversation(id, chooseVariant == RENDER_CONVERSATION_START);
            break;
        case RENDER_CONVERSATION_DELETE:
            beginLoading();

            unsigned* xId = SDL_malloc(sizeof(int));
            *xId = id;
            lifecycleAsync((LifecycleAsyncActionFunction) &deleteConversation, xId, 0);
            break;
        default:
            assert(false);
    }
}

void logicOnServerShutdownRequested(void) {
    assert(this);
    if (!this->netInitialized) return;

    beginLoading();
    lifecycleAsync((LifecycleAsyncActionFunction) &netShutdownServer, NULL, 0);
}

void logicOnReturnFromConversationPageRequested(void) {
    assert(this && this->databaseInitialized);
    renderShowUsersList(this->currentUserName);
}

static void utoa(char* buffer, unsigned length, unsigned number) { // unsigned to ascii (string) (and to utf-8 too as all the english letters are presented in utf-8 the same way as they're in ascii)
    byte* digits = NULL;
    unsigned digitsSize = 0;

    while (number > 0) {
        digits = SDL_realloc(digits, ++digitsSize * sizeof(char));
        digits[digitsSize - 1] = number % 10;
        number /= 10;
    }

    assert(length >= digitsSize);

    for (unsigned i = digitsSize, j = 0; i < length; i++, j++)
        buffer[j] = '0' + 0;
    for (unsigned i = length - digitsSize, j = digitsSize - 1; i < length; i++, j--)
        buffer[i] = (char) ('0' + digits[j]);

    SDL_free(digits);
}

static void monthToName(char* buffer, unsigned monthIndex) {
    assert(monthIndex < 12);
    const char months[12][3] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    SDL_memcpy(buffer, months[monthIndex], 3);
}

char* logicMillisToDateTime(unsigned long millis) {
    millis /= 1000;
    struct tm* tm = localtime((long*) &millis);
    assert(tm);

    // 'hh:mm:ss mmm-dd-yyyy'  20 + \0 = 21
    char* text = SDL_calloc(21, sizeof(char));

    utoa(text, 2, tm->tm_hour);
    text[2] = ':';
    utoa(text + 3, 2, tm->tm_min);
    text[5] = ':';
    utoa(text + 6, 2, tm->tm_sec);
    text[8] = ' ';

    monthToName(text + 9, tm->tm_mon);
    text[12] = '-';
    utoa(text + 13, 2, tm->tm_mday);
    text[15] = '-';
    utoa(text + 16, 4, tm->tm_year + 1900);
    text[20] = 0;

    return text;
}

unsigned long logicCurrentTimeMillis(void) {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return timespec.tv_sec * (unsigned) 1e3f + timespec.tv_nsec / (unsigned) 1e6f;
}

static void sendMessage(void** params) {
    assert(this && params && this->databaseInitialized);

    const unsigned size = *((unsigned*) params[1]);
    assert(size && size <= logicMaxMessagePlainPayloadSize());

    const byte* text = params[0];
    DatabaseMessage* dbMessage = databaseMessageCreate(logicCurrentTimeMillis(), this->toUserId, netCurrentUserId(), text, size);
    assert(databaseAddMessage(dbMessage));
    databaseMessageDestroy(dbMessage);

    unsigned paddedSize;
    byte* paddedText = cryptoAddPadding(&paddedSize, text, size);

    unsigned encryptedSize = cryptoEncryptedSize(paddedSize);
    assert(encryptedSize <= NET_MAX_MESSAGE_BODY_SIZE);

    CryptoCoderStreams* coderStreams = databaseGetConversation(this->toUserId);
    assert(coderStreams);
    byte* encryptedText = cryptoEncrypt(coderStreams, paddedText, paddedSize, false);
    SDL_free(paddedText);
    cryptoCoderStreamsDestroy(coderStreams);

    netSend(NET_FLAG_PROCEED, encryptedText, encryptedSize, this->toUserId);
    SDL_free(encryptedText);

    SDL_free(params[0]);
    SDL_free(params[1]);
    SDL_free(params);
}

void logicOnSendClicked(const char* text, unsigned size) {
    assert(this);
    if (!size) return;

    void** params = SDL_malloc(2 * sizeof(void*));

    params[0] = SDL_malloc(size * sizeof(char));
    SDL_memcpy(params[0], text, size);

    params[1] = SDL_malloc(sizeof(int));
    *((unsigned*) params[1]) = size;

    listAddFront(this->messagesList, conversationMessageCreate(logicCurrentTimeMillis(), NULL, 0, text, size));
    lifecycleAsync((LifecycleAsyncActionFunction) &sendMessage, params, 0);
}

void logicOnUpdateUsersListClicked(void) {
    assert(this && this->databaseInitialized && !this->missingMessagesFetchers);
    beginLoading();

    listClear(this->usersList);
    renderShowUsersList(this->currentUserName);

    lifecycleAsync((LifecycleAsyncActionFunction) &netFetchUsers, NULL, 0);
}

unsigned logicMaxMessagePlainPayloadSize(void) { return (maxUnencryptedMessageBodySize() / CRYPTO_PADDING_BLOCK_SIZE) * CRYPTO_PADDING_BLOCK_SIZE; } // integer (not fractional division) // 136

static long fetchHostId(void) {
    const long dummy = (long) 0xfffffffffffffffFULL; // unsigned long long (= unsigned long)

    SDL_RWops* rwOps = SDL_RWFromFile("/etc/machine-id", "rb");
    if (!rwOps) return dummy;

    const byte size = 1 << 5; // 32
    byte buffer[size];

    const bool sizeMatched = SDL_RWread(rwOps, buffer, 1, size) == size;
    assert(!SDL_RWclose(rwOps));
    if (!sizeMatched) return dummy;

    long hash = 0;
    for (byte i = 0; i < size; hash ^= buffer[i++]);
    return hash;
}

void logicClean(void) {
    assert(this);

    if (this->netInitialized) netClean();

    queueDestroy(this->userIdsToFetchMessagesFrom);

    assert(!this->rwops);
    assert(!this->fileHashState);

    if (this->databaseInitialized) databaseClean();
    optionsClean();
    cryptoClean();

    SDL_free(this->currentUserName);

    listDestroy(this->usersList);
    listDestroy(this->messagesList);

    SDL_free(this);
}
